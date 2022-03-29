# Rain World Background Generator
### Available as a web app at http://isaacelenbaas.us.to/RWBG.html
## About
RWBG will, given monitor information, generate a background where each monitor is a Rain World screen and each screen is (mostly, I did my best) in the correct game-space relative to those around it.

One small issue is that Shaded is very large so black screens are not uncommon, lol

I did my best to make this cross-platform, but PRs are welcome.  
The build system is a bit horrible to embed the images in the binary, but it works and only takes 700s on my laptop with `-O3`.  
`gen_map.py` requires a folder named "Merged Screenshots" with subfolders for each region (in their two-letter names) and screenshots for each room sorted in those. These are available [here](https://github.com/LauraHannah44/Rain-World-Images), although not named properly, as well as in the split archive. You will also need to set its path to your Rain World installation folder.  
Modded regions are supported if one can provide merged screenshots for them.
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

This setup gives you a lot of flexibility - let's say you had one vertical monitor and one horizontal. You could tell RWBG that the vertical monitor is three to encourage putting large vertical rooms on it (and fill more of the space):
```
1 0
1 1
1 0
```
`main 2x3 1,1 1,1@1080,420:1920x1080 0,0@0,0:1080x607 0,1@0,607:1080x607 0,2@0,1214:1080x607`
