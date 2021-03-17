windisplaymode
==============

[![Build status][build_badge_image]][build_page]

It's just a command-line utility that can change resolution and refresh rate of your monitor.

Usage:

```
windisplaymode list <display>
        Print a list of available display modes for the specified display
windisplaymode set <display> <mode>
        Change display mode

Examples:

windisplaymode set 0 1920x1080
        Change resolution of display 0 (first display) to 1920 (width) by 1080 (height) pixels
windisplaymode set 0 1920x1080x32
        Change resolution of display 0 to 1920x1080 with 32-bit colors
windisplaymode set 0 1920x1080@60
        Set both resolution and refresh rate (60 Hz)
windisplaymode set 0 @144
        Change refresh rate to 144 Hz keeping the same resolution and color depth
```

[build_badge_image]: https://ci.appveyor.com/api/projects/status/j8vn5t9c67bu58xu/branch/master?svg=true
[build_page]: https://ci.appveyor.com/project/Zeex/windisplaymode/branch/master
