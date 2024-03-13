const fs = require("fs");
const path = require("path");
const { spawn } = require("child_process");
let RWBG_ESA = spawn("./RWBG-ESA", ["--daemon"], { stdio: ["pipe", "inherit", "inherit"] });
let RWBG_RW  = spawn("./RWBG-RW",  ["--daemon"], { stdio: ["pipe", "inherit", "inherit"] });
process.on("exit", function() {
	RWBG_ESA.kill();
	RWBG_RW.kill();
});

let queue_RWBG_max_active = 3;

let queue_RWBG_running = 0;
let queue_RWBG_queue = [];
function queue_RWBG(res_RWBG) {
	if(queue_RWBG_queue.length > 0 || queue_RWBG_running >= queue_RWBG_max_active)
		queue_RWBG_queue.push(res_RWBG);
	else res_RWBG();
}
function dequeue_RWBG() {
	queue_RWBG_running--;
	let res_RWBG = queue_RWBG_queue.shift();
	if(res_RWBG !== undefined) res_RWBG();
}

module.exports = {
//{{{ get_RWBG(req, res, authorized)
get_RWBG: function(req, res, authorized) {
	if(req.url.startsWith("/RWBG/")) {
		let desc = path.basename(req.url).replace(/\.png$/, "");
		if(function() {
			if(!/^\dx\d \d,\d /.test(desc)) return false;
			for(monitor of desc.replace(/^\dx\d \d,\d /, "").split(" ")) {
				if(!/^\d,\d@\d{1,5},\d{1,5}:\d{1,4}x\d{1,4}$/.test(monitor)) return false;
			}
			return true;
		}()) {
			let res_RWBG = function() {
				let dequeued = req.url.startsWith("/RWBG/ESA/");
				if(!dequeued) queue_RWBG_running++;
				// use this so that RWBG has no chance of trying to write to some random process's stdout if PID were reassigned
				let can_exit_now = false;
				let exit_now = false;
				// have to cat -> cat so that inner's stdout is a pipe RWBG can print to (node uses sockets)
				// cat | cat makes zombie processes (not killed from closing pipes), process substitution prevents that
				let RWBG_sub = spawn("cat <(sh -c \"printf \\\"\\$\\$\\\\n\\\"; exec cat\")", { shell: true, timeout: 120000 });
				RWBG_sub.stdout.once("data", function(pid) {
					let empty = true;
					RWBG_sub.stdout.on("data", function(chunk) {
						if(exit_now && RWBG_sub.exitCode === null) { RWBG_sub.kill(); return; }
						can_exit_now = true;
						if(empty) {
							res.writeHead(200, { "Content-Type": "image/png" });
							empty = false;
						}
						if(!res.write(chunk)) RWBG_sub.stdout.pause();
					});
					res.on("drain", function() { RWBG_sub.stdout.resume(); });
					RWBG_sub.stdout.on("end", function() {
						if(empty) {
							res.writeHead(400);
							res.end("This monitor configuration has no solutions (or there was a server issue)");
						}
						else res.end();
						if(!dequeued) {
							dequeued = true;
							dequeue_RWBG();
						}
					});
					if(req.url.startsWith("/RWBG/ESA/")) RWBG_ESA.stdin.write(pid.toString().slice(0, -1) + " " + desc + "\n");
					else                                  RWBG_RW.stdin.write(pid.toString().slice(0, -1) + " " + desc + "\n");
				});
				req.removeAllListeners("close");
				req.on("close", function() {
					if(can_exit_now && RWBG_sub.exitCode === null) RWBG_sub.kill();
					exit_now = true;
					if(!dequeued) {
						dequeued = true;
						dequeue_RWBG();
					}
				});
			};
			if(req.url.startsWith("/RWBG/ESA/")) res_RWBG();
			else {
				queue_RWBG(res_RWBG);
				req.on("close", function() {
					let i = queue_RWBG_queue.indexOf(res_RWBG);
					if(i !== -1) {
						queue_RWBG_queue.splice(i, 1);
						res.end();
					}
				});
			}
		}
		else {
			res.writeHead(400);
			res.end("This monitor configuration is invalid or too large");
		}
		return true;
	}
}
//}}}
};
