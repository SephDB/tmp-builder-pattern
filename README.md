# TMP Builder pattern
C++17 single-header library for creating a builder for types with mandatory members, statically checking whether all of them are set.
It is fully constexpr-enabled and compiles away to setting all members directly starting on -O2 on both GCC and Clang in non-constexpr usage.

As an illustrative example problem, here's a simple Rectangle struct used in a hypothetical graphics library:
```cpp
struct Rect {
  int x,y,w,h;
  std::array<uint8_t,4> color = {0,0,0,255};
  double rotation = 0.0;
};
```
Here, the x,y,w and h attributes are supposed to be mandatory, while color and rotation have default values and are optional.
You can construct it using a named initializer list and have the code be clear as to what you meant:
```
Rect r = {.x = 0, .y = 10, .h = 15};
```
But this will default-construct all the members you did not mention and for more complicated structs, missing mandatory arguments is an easy mistake to make. As evidenced by me forgetting to set the width above.

A common solution to this problem is the Builder pattern, where a separate class has methods for building the object
```cpp
class RectBuilder {
    Rect inner;
    bool has_x=false,has_y=false,has_w=false,has_h=false;
    public:
        RectBuilder& setX(int x) {inner.x = x;has_x=true;return *this;}
        RectBuilder& setY(int y) {inner.y = y;has_y=true;return *this;}
        RectBuilder& setW(int w) {inner.w = w;has_w=true;return *this;}
        RectBuilder& setH(int h) {inner.h = h;has_h=true;return *this;}
        RectBuilder& setColor(std::array<uint8_t,4> color) {inner.color = color;return *this;}
        RectBuilder& setRotation(double rotation) {inner.rotation = rotation;return *this;}

        Rect& get() {
            if(has_x && has_y && has_h && has_w) {
                return inner;
            } else {
                throw SomeError();
            }
        }
};
Rect r = RectBuilder().setX(10).setY(20).setW(10).setH(10).get();
```
This allows for readable syntax, initialization of members in any order and/or over the course of multiple statements, checking if preconditions are filled(mandatory fields in our case), etc. The major disadvantage is that this is a lot of boilerplate to write for every type you want to apply this technique to, and that without a more complex builder, the check for mandatory arguments has to be done at runtime.

This project simplifies that work to a single alias in the Rect type:
```cpp
struct Rect {
  int x,y,w,h;
  std::array<uint8_t,4> color = {0,0,0,255};
  double rotation = 0.0;
  using Builder = tmp_builder::Builder<Rect,&Rect::x,&Rect::y,&Rect::w,&Rect::h>;
};
Rect r = Rect::Builder().set<&Rect::x>(10).set<&Rect::y>(20).set<&Rect::w>(10).set<&Rect::h>(10).get();
```
The first parameter type is the type you're creating the builder for, and then follows a list of pointers-to-members for all required members.

`.get()` will static_assert if all required members haven't been set.
Example:
```
Rect r = Rect::Builder().set<&Rect::x>(0).set<&Rect::y>(10).set<&Rect::h>(15).get(); //static_assert: error: static_assert failed "Can't get from incompleted build" with type Rect::Builder<Rect,&Rect::w> mentioned in error message
Rect w = Rect::Builder().set<&Rect::x>(0).set<&Rect::y>(10).set<&Rect::w>(15).set<&Rect::h>(15).set<&Rect::color>({0,128,0,200}).get(); //OK
```

## Usage

To use this library, include builder.hpp and create an alias to a builder type with your own type as the first template argument and pointers to the mandatory members as the remaining arguments.
The type is required to be default-constructible. If some members are not, it is possible to pass a partially-constructed object into the Builder's constructor. These non-default-constructible members should be excluded from the mandatory member list as they do not get picked up on having been initialized already.

Feeback would be greatly appreciated.
