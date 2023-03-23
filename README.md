# Rain World Background Generator
### [Example output](http://RWBG.isaacelenbaas.com/RWBG/RW/2x3%200,1%200,1@0,200:1920x1080%200,2@0,1280:1920x1080%201,0@1920,50:1080x607%201,1@1920,657:1080x607%201,2@1920,1264:1080x607) - try refreshing!
The above monitor setup is two stacked horizontal monitors with a vertical one to their right. The vertical monitor is "pretending" to be three stacked screens, see the bottom of this README.
### Available as a web app [here](http://RWBG.isaacelenbaas.com/RWBG.html)
Entering your monitor configuration will redirect you to a link which can be refreshed to generate a new background and/or used in a [script](https://github.com/IsaacElenbaas/dotfiles/blob/master/bin/rollbg) to randomize your background after some time. This background will need to be set to stretch over all displays.
## About
RWBG will, given monitor information, generate a background where each monitor is a game screen and each screen is (mostly, I did my best) in the correct game-space relative to those around it. Letterboxing is avoided where not necessary.

Currently supported games are Rain World and Environmental Station Alpha. Build each using `make RW` or `make ESA`.

I did my best to make this cross-platform compatible, but it is not completely and I do not intend to finish it at this time. PRs are welcome.  
The build system is a bit horrible to embed the images in the binary, but it works and is relatively quick. (EDIT: nevermind, Downpour is huge - definitely compile with `-jN`)  
Rain World's `gen_map.py` requires a folder named "Merged Screenshots" with a specific layout containing merged screenshots of each room in the game. These are available [here](https://github.com/IsaacElenbaas/RWMS). You will also need to set a path in `gen_map.py` to your Rain World installation folder.  
Modded regions are supported if one can provide merged screenshots for them. This can be done with the script alongside the merged screenshots which I used to generate them.
## Usage
`main AxB C,D E,F@G,H:IxJ E,F@G,H:IxJ ...`  
Where:  
	`AxB` are grid dimensions of your monitor setup  
	`C,D` are the grid coordinate of your primary monitor (or at least which to start searching from)  
	`E,F` are the grid coordinate of a monitor  
	`G,H` are the pixel coordinate of a monitor  
	`IxJ` are the pixel dimensions of a monitor  
Coordinates have their origin in the upper left.

For example, my current setup has two monitors above my laptop:
```
1 1
1 0
```
with the following command:
`main 2x2 0,1 0,1@622,1080:1920x1080 0,0@0,0:1920x1080 1,0@1920,0:1920x1080`

This setup also gives you a lot of flexibility - let's say you had one vertical monitor and one horizontal. You could tell RWBG that the vertical monitor is three to encourage putting large vertical rooms on it (and fill more of the space):
```
1 0
1 1
1 0
```
`main 2x3 1,1 1,1@1080,420:1920x1080 0,0@0,0:1080x607 0,1@0,607:1080x607 0,2@0,1214:1080x607`
