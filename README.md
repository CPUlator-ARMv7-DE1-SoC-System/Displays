# 02 - Displays

## Overview

This experiment explores the mathematical principles behind drawing and animating ellipses in the CPUlator ARMv7 DE1-SoC System simulator.

The initial approach uses the equation of an ellipse to determine which pixels lie along its boundary. By varying one of the ellipse's radii over time, the program creates an animation in which the ellipse continuously changes its shape. This demonstrates how a mathematical equation can be translated into pixel-level graphics and used to produce dynamic visual output.

The experiment also explores Bresenham-style rasterization algorithms as an alternative approach to drawing ellipses. Comparing these methods provides an opportunity to examine different ways of translating geometric shapes into discrete pixels on a display.

### 1. VGA Pixel Buffer

The VGA pixel buffer is the region of memory used to represent the image displayed on the screen. In the CPUlator ARMv7 DE1-SoC System simulator, the pixel buffer starts at address `0xC8000000` and represents a display with a resolution of 320 × 240 pixels.

Each pixel is represented by a 16-bit value, meaning that two bytes are required to store its color. Since each row contains 320 pixels, the minimum amount of memory required to store one row is 640 bytes.

However, the memory layout used by the pixel buffer allocates 1024 bytes for each row rather than packing the rows directly next to one another at 640-byte intervals. This provides a power-of-two row stride, which simplifies address calculations because multiplying a row index by 1024 is equivalent to shifting it left by ten bits.

The `getPixel()` function uses this layout to calculate the memory address of a pixel from its coordinates:

```c
#define PIXEL_BUFFER ((uintptr_t) 0xC8000000)

volatile uint16_t *getPixel(int32_t x, int32_t y) {
    uintptr_t address =
        PIXEL_BUFFER + ((uintptr_t)y << 10) + ((uintptr_t)x << 1);

    return (volatile uint16_t *)address;
}
```

The address is calculated by adding three components:

1. **Pixel buffer base address:** `PIXEL_BUFFER` identifies the beginning of the pixel buffer.
2. **Row offset:** `y << 10` multiplies the row index by 1024 bytes, locating the beginning of the corresponding row.
3. **Column offset:** `x << 1` multiplies the column index by two bytes, locating the pixel within that row.

The resulting address points to the pixel's 16-bit color value. The use of `volatile uint16_t *` allows the program to access the pixel data directly in memory.

#### Why `uintptr_t` Is Used

The type `uintptr_t`, defined in `<stdint.h>`, is an unsigned integer type capable of holding a converted pointer value, if the implementation provides this optional type. It is useful when working with memory addresses because it allows an address to be represented as an integer rather than as a pointer.

In this implementation, `PIXEL_BUFFER` is defined using `uintptr_t`, and the address of each pixel is calculated through integer arithmetic. This allows the code to add the row and column offsets directly to the base address before converting the resulting address into a `volatile uint16_t *`.

The bit shifts in the calculation operate on integer values: `y << 10` calculates the row offset in bytes, while `x << 1` calculates the column offset. Once these offsets have been added to the base address, the result is cast to a pointer so that the program can access the pixel's 16-bit color value.

Using `uintptr_t` therefore makes the address calculation explicit: the program first calculates a numerical memory address and then converts it into a pointer to the pixel data.

This addressing scheme connects the two-dimensional coordinates of a pixel to its location in linear memory. By using bit shifts instead of multiplication, the code takes advantage of the power-of-two row stride to express the address calculation compactly.
