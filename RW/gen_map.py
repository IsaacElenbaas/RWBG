#!/usr/bin/python3

import collections
import math
import os
import re
import sys

path = "/media/data0/SteamLibrary/steamapps/common/Rain World"
ignore = { "OE_TEMP": True }

#{{{ class Room(name, x, y, w, h, cameras)
class Room:
	def __init__(self, name, x, y, w, h, cameras):
		# contains [name_other, x, y, x_other, y_other, dir, dir_other]
		# xs and ys are for entrance and in tile units
		# for dir, 0 is right, 1 is up, 2 is left, 3 is down
		self.connections = {}

	#{{{ copy properties to self
		self.name = name
		self.x = x
		self.y = y
		# tile units
		self.w = w
		self.h = h
	#}}}

	#{{{ create Screens from cameras
		self.screens = [[int(i[0]), int(i[1].strip())] for i in cameras]
		for i in range(len(self.screens)-1, -1, -1):
			# assume the first camera is good
			# almost all larger rooms are only expanded in one direction and this doesn't fail any good ones at the moment
			if abs(self.screens[i][0]-self.screens[0][0]) > 3*1024 and \
			   abs(self.screens[i][1]-self.screens[0][1]) > 3*768:
				print("Skipping camera {} in room {}".format(i, name), file=sys.stderr)
				self.screens.pop(i)
			else:
				self.screens[i][1] = 20*h-800-self.screens[i][1]

		#{{{ rambling to make sense of coordinates and offsets
				# plotting sum of (offsets from first camera % x) near expected possible screen dimensions:
				# 	y=c1+c2x+c3/(1+e^(c4(1034-x))) for x offsets
				# 	y=c1+c2x+c3/(1+e^(c4(738-x)))  for y offsets
				# both with R^2=0.999
				# so it looks like map files are in pixels in 1024x768, the default resolution
				#
				# a standard room is 48x35 tiles with a -220,-30 offset camera
				# (with y translation that's a -220,-70 offset camera)
				# 	one tile unit is 20 pixels -> 960x700
				# 	64 extra x pixels and 68 extra y pixels on"screen"
				# 	-64/2,-68/2 offset to center tiles
				# 	game renders at 1400x800
				# 	-(1400-1024)/2,-(800-768)/2 offset to center room
				# =-220,-50
				# so tile origin is the same as the camera origin and we can ignore resolution
				#
				# we have to remember that the offset determines the lower left of the 1400x800 canvas, though
				# game view is (mostly) centered on the 1400x800 render (up 2 more than halfway, 18, from bottom, +-8 screen shake)
				# (1400-1366)/2,18 (the screenshots are at max res, 1366x768) ->  17,18 offset for screenshots
				# (1400-1024)/2,18                                            -> 188,18 offset for unique screen
				# (1366-1024)/2, 0                                            -> 171, 0 offset for unique screen from screenshot
		#}}}

				screens[name + "_" + str(i)] = Screen(
					name + "_" + str(i),
					x+(self.screens[i][0]+188)/20, y+(self.screens[i][1]+18)/20, # map coords of bottom left of unique screen
					# 171 isn't actually doing anything here, it is eliminated and reapplied when making relative to bottom left screen just below this
					self.screens[i][0]+171, self.screens[i][1] # screenshot coords of bottom left of unique screen
				)
				self.screens[i] = screens[name + "_" + str(i)]

		#{{{ remove universal screen offset as it wouldn't affect the screenshot
		x_scrot_min = math.inf;
		y_scrot_min = math.inf;
		y_scrot_max = -math.inf;
		for screen in self.screens:
			x_scrot_min = min(x_scrot_min, screen.x_scrot)
			y_scrot_min = min(y_scrot_min, screen.y_scrot)
			y_scrot_max = max(y_scrot_max, screen.y_scrot)
		y_scrot_max -= y_scrot_min
		for screen in self.screens:
			screen.x_scrot -= x_scrot_min-171
			screen.y_scrot -= y_scrot_min
			screen.y_scrot = y_scrot_max-screen.y_scrot
		#}}}
	#}}}
#}}}

#{{{ class Screen(name, x_map, y_map, x_scrot, y_scrot)
class Screen:
	def __init__(self, name, x_map, y_map, x_scrot, y_scrot):
		# contains [name_other, dir bitmap]
		self.connections = []
		self.campaigns = {}
		self.name = name
		self.x_map = x_map
		self.y_map = y_map
		self.x_scrot = x_scrot
		self.y_scrot = y_scrot
#}}}

path = re.sub('/*$', "/RainWorld_Data/StreamingAssets/", path)
slugcats = ["", "gourmand", "artificer", "rivulet", "spear", "saint"]
rooms = {}
screens = collections.OrderedDict()

#{{{ load everything
try:
	main = open(path + "world/regions.txt")
	msc = open(path + "mods/moreslugcats/modify/world/regions.txt")
	regions = set(main.readlines() + [re.sub('^.*]', "", region) for region in msc.readlines()])
	main.close()
	msc.close()
except IOError as e:
	print(e, file=sys.stderr)
	print("You may have supplied an incorrect path", file=sys.stderr)
	print("Also note that Downpour is required for this script", file=sys.stderr)
	exit(1)
for slugcat in slugcats:
	for region in regions:
		region = region.strip()
		if not re.match('^[A-Z]{2,}$', region):
			print("Skipping region {}".format(region), file=sys.stderr)
			continue
		try:
			region_map = open(path + "mods/moreslugcats/world/{r}/map_{r}{}{}.txt".format("-" if slugcat != "" else "", slugcat, r=region.lower()), "r")
		except IOError as e:
			#print("Skipping region {} as {}".format(region, slugcat), file=sys.stderr)
			continue
		for line in region_map:
			title, data = line.split(":")
			data = data.strip()
			if title.split("_")[0] in ["OffScreenDen"]:
				continue
			if title != "Connection":
				if title in ignore:
					continue

	#{{{ parse room
				x_map, y_map, x_dev, y_dev, layer, subregion, w, h = data.split("><")
				# convert to tile units
				x_map = float(x_map)/2; y_map = float(y_map)/2
				x_dev = float(x_dev)/3; y_dev = float(y_dev)/3
				try:
					try:
						if title.split("_")[0] != "GATE":
							room_map = open(path + "mods/moreslugcats/world/{}-rooms/{}.txt".format(region.lower(), title.lower()), "r")
						else:
							room_map = open(path + "mods/moreslugcats/world/gates/{}.txt".format(title.lower()), "r")
					except IOError as e:
						if title.split("_")[0] != "GATE":
							room_map = open(path + "world/{}-rooms/{}.txt".format(region.lower(), title.lower()), "r")
						else:
							room_map = open(path + "world/gates/{}.txt".format(title.lower()), "r")
					room_map.readline() # name
					w, h = room_map.readline().split("|")[0].split("*") # rest of this line is water info
					w = int(w); h = int(h)
					# map coordinates are for center
					x_map = x_map-w/2
					y_map = y_map-h/2
					room_map.readline() # light angle
					cameras = [i.split(",") for i in room_map.readline().split("|")]
					room_map.close()
				except Exception as e:
					print(e, file=sys.stderr)
					print("Skipping room in {}: {}".format(region, title), file=sys.stderr)
					continue
				rooms[title] = Room(title, x_dev, y_dev, w, h, cameras)
	#}}}

			else:

	#{{{ parse connection
				room_a, room_b, x_a, y_a, x_b, y_b, dir_a, dir_b = data.split(",")
				if room_a.split("_")[0] in ["OffScreenDen", "DISCONNECTED"] or \
					 room_b.split("_")[0] in ["OffScreenDen", "DISCONNECTED"]:
					continue
				try:
					if room_b not in rooms[room_a].connections:
						rooms[room_a].connections[room_b] = [room_b, float(x_a), float(y_a), float(x_b), float(y_b), int(dir_a), int(dir_b)]
				except Exception as e:
					print(e, file=sys.stderr)
					print("Skipping connection in {}: {}".format(region, data), file=sys.stderr)
	#}}}

		region_map.close()
#}}}

#{{{ create connections with possible directions between screens
for room in rooms.values():
	for connection in room.connections.values():
		rooms_con = [room, rooms[connection[0]]]
		screens_con = []

	#{{{ get the screens for each end of pipe
		for i in [0, 1]:
			x = rooms_con[i].x+connection[1+2*i]
			y = rooms_con[i].y+connection[2+2*i]
			best_err = sys.float_info.max
			for screen in rooms_con[i].screens:
				if x > screen.x_map and x < screen.x_map+1024/20 and \
					 y > screen.y_map and y < screen.y_map+ 768/20:
					break
				else:
					if (err :=
						max(0, screen.x_map-x, x-(screen.x_map+1024/20))+
						max(0, screen.y_map-y, y-(screen.y_map+ 768/20))
					) < best_err:
						best_err = err
						best = screen
			else:
				if best_err > 1:
					print("Using entirely offscreen (by {}px) connection: {},{}".format(round(best_err*20-20), rooms_con[i].name, rooms_con[1-i].name), file=sys.stderr)
				screen = best
			screens_con.append(screen)
	#}}}

		for i in [0, 1]:
			# for given pipe directions, 0 is right, 1 is up, 2 is left, and 3 is down
			# same assignments but they're bit positions
			directions = 0b0000

	#{{{ get possible directions
			# don't allow connections coming from gates (stay in one region)
			if rooms_con[i].name.split("_")[0] == "GATE":
				continue
			if connection[5+i] != connection[6-i]:
				# right left/up down, obvious
				if connection[5+i]%2 == connection[6-i]%2:
					directions |= 0b0001 << connection[5+i]
				# for diagonal connections just give up and allow any direction except behind pipe
				else:
					directions |= 0b1111 ^ (0b0001 << ((connection[5+i]+2)%4))
			else:
				if connection[5+i]%2 == 0:

		#{{{ right right/left left, either (up or down) or (up and down)
					# compare pipes' locations, not rooms', to see if they're mostly on top of each other
					# definitely shouldn't be more lenient, not sure if should be more strict
					if abs((rooms_con[i].x+connection[1+2*i])-(rooms_con[1-i].x+connection[3-2*i])) < (1024/20)/4:
						if screens_con[i].y_map-screens_con[1-i].y_map < 0:
							directions |= 0b0010
						else:
							directions |= 0b1000
					else:
						directions |= 0b1010
		#}}}

				else:

		#{{{ same thing for up up/down down
					if abs((rooms_con[i].y+connection[2+2*i])-(rooms_con[1-i].y+connection[4-2*i])) < (768/20)/4:
						if screens_con[i].x_map-screens_con[1-i].x_map < 0:
							directions |= 0b0001
						else:
							directions |= 0b0100
					else:
						directions |= 0b0101
		#}}}
	#}}}

			screens_con[i].connections.append([screens_con[1-i].name, directions])

	#{{{ add connections between screens in same room
	for i in range(len(room.screens)):
		for j in range(i+1, len(room.screens)):
			if abs(room.screens[i].x_scrot-room.screens[j].x_scrot) < 1024/4:
				if 768/2 < abs(room.screens[i].y_scrot-room.screens[j].y_scrot) < 768+768/2:
					if room.screens[i].y_scrot > room.screens[j].y_scrot:
						room.screens[i].connections.append([room.screens[j].name, 0b0010])
						room.screens[j].connections.append([room.screens[i].name, 0b1000])
					else:
						room.screens[i].connections.append([room.screens[j].name, 0b1000])
						room.screens[j].connections.append([room.screens[i].name, 0b0010])
			elif abs(room.screens[i].y_scrot-room.screens[j].y_scrot) < 768/4:
				if 1024/2 < abs(room.screens[i].x_scrot-room.screens[j].x_scrot) < 1024+1024/2:
					if room.screens[i].x_scrot < room.screens[j].x_scrot:
						room.screens[i].connections.append([room.screens[j].name, 0b0001])
						room.screens[j].connections.append([room.screens[i].name, 0b0100])
					else:
						room.screens[i].connections.append([room.screens[j].name, 0b0100])
						room.screens[j].connections.append([room.screens[i].name, 0b0001])
	#}}}
#}}}

#{{{ create C files
path = (re.sub('/.*?$', "", __file__) or ".") + "/"
map_h = open(path + "map.h", "w")
map_c = open(path + "map.c", "w")
print("#ifndef map_H\n#define map_H", file=map_h)
print("#include <stdlib.h>\n#include <string.h>", file=map_h)
print(
'''
typedef struct Screenshot {
	unsigned char* volatile blob;
	int length;
} Screenshot;
'''
, file=map_h)
print(
'''int w_scrot = 1024;
int w_scrot_extended = 1366;
int h_scrot = 768;
'''
, file=map_c)

	#{{{ create room Screenshots
for room in rooms:
	found = False
	for slugcat in slugcats:
		if room.split("_")[0] == "GATE":
			folder = room.split("_")[1]
		else:
			folder = room.split("_")[0]
		folder = ("white" if slugcat == "" else slugcat) + "/" + folder
		if not os.path.isfile(path + "Merged Screenshots/" + folder + "/" + room + ".png"):
			continue
		found = True
		for screen in rooms[room].screens:
			screen.campaigns[slugcat] = True
		print("extern Screenshot s_{}_{};".format(slugcat, room), file=map_h)
		print("void init_s_{}_{}();".format(slugcat, room), file=map_h)
		print("init_s_{}_{}();".format(slugcat, room), file=map_c)
		if not os.path.isfile(path + "Merged_Screenshots_C/" + folder + "/" + room + ".c"):
			print("Creating {}".format("Merged_Screenshots_C/" + folder + "/" + room + ".c"), file=sys.stderr)
			screenshot = open(path + "Merged Screenshots/" + folder + "/" + room + ".png", "rb")
			os.makedirs(path + "Merged_Screenshots_C/" + folder, exist_ok=True)
			screenshot_c = open(path + "Merged_Screenshots_C/" + folder + "/" + room + ".c", "w")
			print("#include \"map.h\"", file=screenshot_c)
			print("Screenshot s_{}_{};".format(slugcat, room), file=screenshot_c)
			print("void init_s_{}_{}() {{".format(slugcat, room), file=screenshot_c)
			print("\ts_{}_{}.length = {};".format(slugcat, room, os.path.getsize(path + "Merged Screenshots/" + folder + "/" + room + ".png")), file=screenshot_c)
			print("\ts_{s}_{r}.blob = malloc(s_{s}_{r}.length*sizeof(char));".format(s=slugcat, r=room), file=screenshot_c)
			print("\tunsigned char* s = s_{}_{}.blob;".format(slugcat, room), file=screenshot_c)
			i=0
			close = False
			while (byte := screenshot.read(1)):
				if i%1024 == 0:
					close = True
					print("\tmemcpy(s+{}*sizeof(char),\"".format(i), end="", file=screenshot_c)
				print("\\x{:x}".format(int.from_bytes(byte, 'big')), end="", file=screenshot_c)
				if (i := i+1)%1024 == 0:
					close = False
					print("\", 1024*sizeof(char));", file=screenshot_c)
			if close:
				print("\", {}*sizeof(char));".format(i%1024), file=screenshot_c)
			print("}", file=screenshot_c)
			screenshot.close()
			screenshot_c.close()
		else:
			found = True
	if not found:
		print("Missing screenshot for {}".format(room), file=sys.stderr)
		print(path + "Merged Screenshots/?/" + room.split("_")[0] + "/" + room + ".png")
		exit(1)
	#}}}

print("#endif", file=map_h)
map_h.close()

print("", file=map_c)
print("const int screens_length = {};".format(len(screens)), file=map_c)
print("Screen screens[screens_length];", file=map_c)
print("Creating Screens and Connections", file=sys.stderr)
i=-1
for screen in screens:
	screens[screen].index = (i := i+1)
for screen in screens:
	print("", file=map_c)

	#{{{ create Screens
	for slugcat in slugcats:
		# shush and let me write my bad code
		i = slugcats.index(slugcat)
		if i == 0:
			if slugcat in screens[screen].campaigns:
				print("screens[{}].screenshot = &s_{}_{};".format(screens[screen].index, slugcat, re.sub('_\d*$', "", screen)), file=map_c)
			else:
				print("screens[{}].screenshot = NULL;".format(screens[screen].index), file=map_c)
		else:
			if slugcat in screens[screen].campaigns:
				print("screens[{}].screenshot{} = &s_{}_{};".format(screens[screen].index, i, slugcat, re.sub('_\d*$', "", screen)), file=map_c)
			else:
				print("screens[{}].screenshot{} = screens[{}].screenshot;".format(screens[screen].index, i, screens[screen].index), file=map_c)
	print("screens[{}].x_scrot = {};".format(screens[screen].index, screens[screen].x_scrot), file=map_c)
	print("screens[{}].y_scrot = {};".format(screens[screen].index, screens[screen].y_scrot), file=map_c)
	print("screens[{}].connections_length = {};".format(screens[screen].index, len(screens[screen].connections)), file=map_c)
	print("screens[{s}].connections = malloc(screens[{s}].connections_length*sizeof(Connection));".format(s=screens[screen].index), file=map_c)
	#}}}

	#{{{ create Connections
	j=0
	for connection in screens[screen].connections:
		print("screens[{}].connections[{}].screen = &screens[{}];".format(screens[screen].index, j, screens[connection[0]].index), file=map_c)
		print("screens[{}].connections[{}].direction = {};".format(screens[screen].index, j, connection[1]), file=map_c)
		j += 1
	#}}}

map_c.close()
#}}}
