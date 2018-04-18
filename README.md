# TMP Builder pattern
Class for creating a compiletime-checked builder of a struct type

As an illustrative example problem, here's a simple Rectangle struct used in a hypothetical graphics library:
```cpp
struct Rect {
  int x,y,w,h;
  std::array<uint8_t,4> color = {0,0,0,255};
  double rotation = 0.0;
};
```
Here, the x,y,w and h attributes are supposed to be mandatory, while color and rotation have default values and are optional.
You can easily construct it using a named initializer list and have the code be clear:
```
Rect r = {.x = 0, .y = 10, .h = 15};
```
But it will simply default-construct all the members you did not mention and for more complicated structs, it can get easy to miss some. As evidenced by me forgetting to set .w above.

With this project, Rect can easily define a Rect builder that statically checks whether all required parameters have been set:
```cpp
struct Rect {
  int x,y,w,h;
  std::array<uint8_t,4> color = {0,0,0,255};
  double rotation = 0.0;
  using Builder = tmp_builder::Builder<Rect,&Rect::x,&Rect::y,&Rect::w,&Rect::h>;
};
```

`tmp_builder::Builder` has two methods: `.set<memPtr>(value)` for each of the members and `.get()` to extract the finished struct, which will static_assert if all required members haven't been set.
Example usage:
```
Rect r = Rect::Builder().set<&Rect::x>(0).set<&Rect::y>(10).set<&Rect::h>(15).get(); //static_assert: error: static_assert failed "Can't get from incompleted build" with type Rect::Builder<Rect,&Rect::w> mentioned in error message
Rect w = Rect::Builder().set<&Rect::x>(0).set<&Rect::y>(10).set<&Rect::w>(15).set<&Rect::h>(15).set<&Rect::color>({0,128,0,200}).get(); //OK
```

If there are non-default-constructible members in the struct, you can pass a partially-constructed struct to the Builder constructor.

On -O2 on both clang and gcc, all traces of this class compile away, and the class is fully constexpr-enabled.
