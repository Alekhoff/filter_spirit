# actual filter language

The following document aims to outline hidden rules of the actual filter language that the Path of Exile game client is parsing.

[The article on wiki](https://pathofexile.gamepedia.com/Item_filter) is generally correct but there are some hidden, undocumented implementation details in game's client parser that have visible consequences but are nowhere mentioned.

GGG does not maintain any dedicated documentation. All information is gathered from official forum posts, annoucements, reddit and my experiments.

## bugs

- Game can issue "Your Item Filter is out of date" instead of "Failed to load Item Filter" in numerous cases of syntax errors.
- Fossils are classified as `Stackable Currency`. They do not actually stack in game.
- Resonators can have 1-4 sockets and Item Filter detects that correctly. Jeweller's orb errors on resonators reporting that they have no sockets.
- `#` starts a comment untill end of line. `#` is not accepted in some contexts:

```
Show
	# similar thing happens for any condition
	# "#" is basically tried to be parsed as some number/keyword/string
	Class     "One"  # error: no item classes matching "#"
	ItemLevel 80     # error: integer "#" not recognised

	# actions:
	SetTextColor       240 200 150     # this comment works
	SetTextColor       240 200 150 255 # this comment works
	SetBorderColor     240 200 150     # this comment works
	SetBorderColor     240 200 150 255 # this comment works
	SetBackgroundColor 255   0   0     # this comment works
	SetBackgroundColor 255   0   0 255 # this comment works
	SetFontSize        42          # error: to many arguments
	MinimapIcon        0  Red Star # error: to many arguments
	PlayEffect         Yellow      # error: to many arguments
	DisableDropSound               # this comment works
	PlayAlertSound           1     # this comment works
	PlayAlertSound           1 300 # this comment works
	PlayAlertSoundPositional 1     # this comment works
	PlayAlertSoundPositional 1 300 # this comment works
	CustomAlertSound "airhorn.wav" # this comment works
```

## implementation details

Generally it's adviced not to rely on implementation details. Listing for exposure only.

- It is possible to specify font size that is outside allowed range (\[18, 42\]). If so happens, the value is clamped in this range. No such thing happens for volume (must be in range \[0, 300\])
- For some conditions, it is possible to specify multiple values (see quality examples below)
- Number parsing:
  - allows optional `+` or `-` character
  - stops on first non-digit character (without an error)

```
Show
	# will match an item with ilvl 10 or 20 or 30
	ItemLevel 10 20 30
	# will match an item with quality < 15
	Quality < 5 10 15
	# syntax error (> is not an integer)
	Quality < 7 > 11
```

expression             | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11
-----------------------|---|---|---|---|---|---|---|---|---|---|----|----
Quality < 5 7 9 11     | + | + | + | + | + | + | + | + | + | + | +  |
Quality < 11 9 7 5     | + | + | + | + | + | + | + | + | + | + | +  |
Quality < 7 11 7 9     | + | + | + | + | + | + | + | + | + | + | +  |
Quality < 4 8          | + | + | + | + | + | + | + | + |   |   |    |
Quality < 8 4          | + | + | + | + | + | + | + | + |   |   |    |
Quality 0 2 4 6 8      | + |   | + |   | + |   | + |   | + |   |    |
Quality +011           |   |   |   |   |   |   |   |   |   |   |    | +
Quality <= 0xB3X 4#pop | + | + | + | + | + |   |   |   |   |   |    |

## other observations

- `SocketGroup` accepts only RGBW letters, however, when you ctrl+C an item in game abyss sockets are denoted with A and fossil sockets (on resonators) are denoted with D
- `Hide` on some items is ignored (usually items of new category introduced by newest game content) (in the past: shaper and elder bases). It is intentional from GGG side - the motivation is to force new game content to be visible in case an outdated filter accidentally hides something important. The drawback is that advanced users can not hide some truly unwanted items.
- Filters were/are never meant to be backwards-compatible. Any future game patch can break existing filters.
- `""` is a valid string. It is consistent with other (non-empty) strings: rules that have empty strings will match all items because any item name also contains an empty string.
- Game client assumes that the filter file is in unicode. [BOM](https://en.wikipedia.org/wiki/Byte_order_mark) is accepted but not required. ASNI encoding breaks where it differs from UTF-8 such as `ö` in `Maelström Staff`. If you get an error that some gibberish string could not be parsed convert file's encoding to UTF-8.