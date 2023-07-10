ModernGlad
==========

It is notably hard to keep track of what "modern OpenGL" means, provided the vast amount of outdated OpenGL tutorials around. Plus, even slightly outdated, some are still very relevant, so no need to dump them away. What if there would just be some **compile time message suggesting what more modern function to look at** when using some outdated function? Well this is the goal of this little companion for the [glad](https://github.com/Dav1dde/glad) OpenGL loader library.

The general rule of thumb to do proper OpenGL nowadays (2020) is tu use *Direct State Access* rather than stateful functions "bind" ones.

Usage
-----

**This is not a replacement for glad.** It is rather a companion, you still have to include glad to your project when using ModernGlad. Your main `CMakeLists.txt` will look like:

```
add_subdirectory(glad)
add_subdirectory(modernglad)
add_subdirectory(my_great_project)
```

and wherever you were using `target_link_library(my_great_project PRIVATE glad)`, replace it with `target_link_library(my_great_project PRIVATE modernglad)`!

To get depreciation warnings, include `glad/modernglad.h` instead of `glad/glad.h` (you can still use the latter wherever you don't want warnings).

Requirements
------------

This requires Python to generate the depreciation header, but since you are using glad already, this is not an extra dependency. If you want a static version of the generated header file, check out the release section on GitHub.

We also require C++14 or above, because before then the `[[depreciated]]` attribute was not supported.
