# 02 - Displays

<br>

## Overview

This experiment explores the mathematical principles behind drawing and animating ellipses in the CPUlator ARMv7 DE1-SoC System simulator.

The initial approach uses the equation of an ellipse to determine which pixels lie along its boundary. By varying one of the ellipse's radii over time, the program creates an animation in which the ellipse continuously changes its shape. This demonstrates how a mathematical equation can be translated into pixel-level graphics and used to produce dynamic visual output.

The experiment also explores Bresenham-style rasterization algorithms as an alternative approach to drawing ellipses. Comparing these methods provides an opportunity to examine different ways of translating geometric shapes into discrete pixels on a display.

<br>

## Objectives

* Controlled and save placing and reading of the pixel inside the buffer
* Different approaches for different purposes: Equational vs Bresenham
* Function-Pointer
* Controlling timed events

<br>

## Control Scheme

The current implementation uses the switches to select the display mode and the push buttons to trigger different ellipse drawing and animation routines.

| Input | Behavior |
|---|---|
| Switch 0 ON, Switch 1 OFF | Enables the first control mode. Push buttons select the equation-based ellipse animation or one of the Bresenham-style ellipse animations. |
| Switch 0 OFF, Switch 1 ON | Enables the second control mode, using the same push-button functions but drawing the grid as the background after the equation-based ellipse animation. |
| Push button 0 pressed (`1`) | Runs the equation-based animated ellipse using `centeredEllipse()`, then clears the background in the first mode or redraws the grid in the second mode. |
| Push button 1 pressed (`2`) | Runs a white Bresenham-style ellipse animation with an angle index of `16`. |
| Push button 2 pressed (`4`) | Runs a white Bresenham-style ellipse animation with an angle index of `8`. |
| Push button 3 pressed (`8`) | Animates two Bresenham-style ellipses using different colors and angle indices. The relative angle is incremented before the animation. |
| Push buttons 0 and 3 pressed (`9`) | Runs the multiple-ellipse animation through `animateMultTraceEllipsesBres()`, incrementing the relative angle before the animation. |
| No recognized button combination in the first mode | No drawing or animation function is called. |
| No recognized button combination in the second mode | Draws the grid. |
| Both switches OFF (`0`) | Clears the pixel buffer. |
| Both switches ON (`3`) | No drawing or animation function is called by the switch-mode branches. |
| Switch register value greater than `3` | Terminates the program by returning `1`. |

The two switch modes share most of their animation routines. Their main difference is the background behavior following the equation-based ellipse animation and when no recognized button combination is pressed in the second mode.

*Note: The implementation checks for exact push-button register values of `1`, `2`, `4`, `8`, and `9`. Other button combinations do not trigger the corresponding animation routines.*

<br>

## Implementation

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

<br>

### 2. Approaches to Ellipse Drawing

#### Equation-Based Approach

The initial implementation uses the mathematical equation of an ellipse to determine which pixels should be colored. By iterating through the entire pixel buffer and evaluating each pixel's position relative to the ellipse equation, the program identifies the pixels that form the ellipse's boundary.

To animate the ellipse, the program changes its radius for each frame and repeats the drawing process. After each frame, the pixel buffer is cleared before the next ellipse is drawn, allowing the animation to progress through a sequence of different ellipse sizes.

This approach worked well in the CPUlator simulator and provided a straightforward way to translate a mathematical equation into a graphical representation. However, iterating through all 320 × 240 pixels for every frame introduces a considerable amount of work, even though only a small portion of the screen belongs to the ellipse's boundary. As a result, the animation was relatively slow, and the individual frames could visibly be seen being drawn across the screen.

#### Bresenham-Based Approach

The performance limitations of the equation-based approach motivated the exploration of Bresenham-style rasterization algorithms. Instead of checking every pixel in the buffer, the algorithm calculates the points along the ellipse's boundary and draws only those pixels.

By avoiding a full-screen iteration for every ellipse, this approach reduces the amount of pixel processing required for each frame. The program can therefore draw and erase ellipses more efficiently, making the animation appear smoother and reducing the visibility of the individual drawing steps.

The experiment consequently demonstrates two different approaches to rasterizing an ellipse: evaluating a mathematical equation across the pixel buffer and calculating boundary points using a Bresenham-style algorithm. Comparing them illustrates how the choice of drawing algorithm affects the performance and visual behavior of an animation.

<br>

### 3. Implementation of the Drawing Approaches

The two ellipse-drawing approaches are integrated into the program as separate visualization modes. Both can be used with either a black background or a grid, allowing the differences in their drawing and clearing strategies to be explored.

#### Equation-Based Ellipse Animation

The equation-based approach is implemented in `centeredEllipse()`. For every frame, the `animate()` function iterates through the entire pixel buffer and passes each pixel to the drawing function. The ellipse equation is evaluated for each pixel, and pixels that satisfy the boundary condition are colored white.

After the animation step, the program explicitly redraws the background. Depending on the selected switch mode, it either clears the pixel buffer using `clear()` or draws the grid using `grid()`.

This separation between drawing the ellipse and rendering the background allows the equation-based approach to work with different background patterns. The drawing function only needs to identify the pixels belonging to the ellipse, while the background can be rendered independently afterward.

The main disadvantage is again, performance. Since every pixel in the 320 × 240 buffer is examined for each frame, the program performs many calculations that do not contribute directly to the ellipse itself.

#### Bresenham-Based Ellipse Animation

The Bresenham-based approach uses `ellipseBres()` to calculate and draw points along the ellipse's boundary. Instead of examining the entire pixel buffer, the algorithm focuses on the points required to represent the ellipse.

The animation functions, such as `animateEllipseBres()` and `animateMultEllipsesBres()`, first draw the ellipse in its designated color, introduce a delay, and then draw the same ellipse again in black. This effectively erases the previous frame without clearing or redrawing the entire pixel buffer.

This approach reduces the amount of drawing required and allows the ellipse to be updated more quickly. However, it introduces an important limitation: **drawing over the previous ellipse with black assumes that the background underneath it is black**. If the ellipse overlaps a grid or another non-black background pattern, those pixels are replaced with black rather than restored to their original colors.

Consequently, the current Bresenham-based animation is best suited to a static, black background unless additional logic is introduced to preserve the background.

#### Background Modes: Black and Grid

The program provides two background modes through the switch configuration.

These modes illustrate a trade-off between the two approaches.

#### Possible Extension: Preserving Background Pixels

One way to extend the Bresenham-based implementation would be to preserve the original color of each pixel before drawing the ellipse over it. When the ellipse needs to be erased, the program could restore the saved color instead of writing black.

For example, the program could store the previous color of each affected pixel in a separate buffer. When drawing an ellipse, it would save the background color before replacing that pixel with the ellipse color. When erasing the ellipse, it would restore the saved value.

This would allow the Bresenham-based approach to work with more complex backgrounds, including the grid, without having to redraw the entire screen after every frame. It would, however, require additional memory and careful handling of pixels that are covered by multiple overlapping shapes.

This is only one possible extension. Readers are encouraged to explore their own approaches to background preservation and rendering, using the current implementation as a starting point.
