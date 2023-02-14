# Rain World Background Generator
### Available as a web app [here](http://isaacelenbaas.us.to/RWBG.html) or [here](http://141.219.194.67/RWBG.html)
Entering your monitor configuration will redirect you to a link which can be refreshed to generate a new background and/or used in a [script](https://github.com/IsaacElenbaas/dotfiles/blob/master/bin/rollbg) - here is [mine.](http://141.219.194.67/RWBG/2x2%200,0%200,0@0,0:1920x1080%201,0@1920,0:1920x1080%200,1@0,1080:1920x1080.png)
## About
RWBG will, given monitor information, generate a background where each monitor is a game screen and each screen is (mostly, I did my best) in the correct game-space relative to those around it. Letterboxing is avoided where not necessary.

Currently supported games are Rain World and Environmental Station Alpha. Build each using `make RW` or `make ESA`.

One small issue with Rain World backgrounds is that Shaded is very large so black screens are not uncommon, lol

I did my best to make this cross-platform compatible, but it is not completely and I do not intend to finish it at this time. PRs are welcome.  
The build system is a bit horrible to embed the images in the binary, but it works and is relatively quick.  
Rain World's `gen_map.py` requires a folder named "Merged Screenshots" with subfolders for each region (in their two-letter names) and screenshots for each room sorted in those. These are available [here](https://github.com/LauraHannah44/Rain-World-Images), although not named properly, as well as in the split archive. You will also need to set a path in `gen_map.py` to your Rain World installation folder.  
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
