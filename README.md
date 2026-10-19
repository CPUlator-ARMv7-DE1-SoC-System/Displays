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

<br>

### 4. Equation-Based Ellipse Animation

The equation-based approach uses three functions to create the animation: `draw()`, `animate()`, and `centeredEllipse()`. Together, they demonstrate how a mathematical equation can be translated into individual pixel operations and how function pointers can be used to make drawing routines reusable.

#### Function Pointers in `draw()` and `animate()`

Both `draw()` and `animate()` iterate through the entire pixel buffer, visiting every pixel from the top-left corner to the bottom-right corner. However, rather than defining what should happen to each pixel themselves, they receive a function pointer as an argument. This allows the caller to decide how each pixel should be processed.

For example, the `draw()` function is defined as follows:

```c
void draw(void (*function)(int16_t, int16_t, volatile uint16_t*)) {
    for (int y = 0; y < PIXEL_BUFFER_HEIGHT; y++) {
        for (int x = 0; x < PIXEL_BUFFER_WIDTH; x++) {
            volatile uint16_t *pixel = getPixel(x, y);
            function(x, y, pixel);
        }
    }
}
```

The parameter `function` is a function pointer. It points to a function that accepts three arguments: the pixel's x-coordinate, its y-coordinate, and a pointer to its 16-bit pixel value. The function must return `void`, meaning it does not return a value.

Inside the nested loops, `draw()` calculates the address of each pixel using `getPixel()` and passes the coordinates and pixel pointer to the supplied function. The expression `function(x, y, pixel)` then calls the function through the pointer.

This means that the same iteration logic can be reused for different drawing operations. For example, calling `draw(clear)` processes every pixel using the `clear()` function, while calling `draw(grid)` processes every pixel using `grid()`.

The `animate()` function uses the same principle but also passes a frame value, `tick_count`, to the drawing function:

```c
void animate(
    void (*function)(int16_t, int16_t, volatile uint16_t*, uint32_t),
    uint32_t tick_count,
    uint32_t delta_time
) {
    for (int y = 0; y < PIXEL_BUFFER_HEIGHT; y++) {
        for (int x = 0; x < PIXEL_BUFFER_WIDTH; x++) {
            volatile uint16_t *pixel = getPixel(x, y);
            function(x, y, pixel, tick_count);
        }
    }
    delay(delta_time);
}
```

Here, the function pointer refers to a function that accepts the same three pixel-related arguments as before, together with a fourth argument containing the current tick count. Once every pixel has been processed, `animate()` calls `delay()` to pause before returning.

In this implementation, `animate()` does not itself define an animation or determine which pixels belong to a shape. Instead, it provides the iteration, passes the current animation state to the drawing function, and introduces a delay. The supplied function determines how that state affects the image.

This separation is useful because the iteration and timing logic can be reused without having to rewrite it for every drawing algorithm.

#### Drawing an Ellipse with `centeredEllipse()`

The function `centeredEllipse()` provides the pixel-processing logic for the equation-based approach. It uses the mathematical equation of an ellipse to determine whether a pixel lies sufficiently close to the ellipse's boundary.

A centered ellipse can be described by the equation:

```math
\frac{x^2}{a^2}+\frac{y^2}{b^2}=1
```

Here, $a$ and $b$ represent the horizontal and vertical radii. For a pixel at coordinates $(x,y)$, the equation evaluates to approximately $1$ when the pixel lies on the ellipse's boundary.

Since the ellipse is centered on the display, the implementation first calculates the absolute distances between the current pixel and the center of the screen:

```c
int32_t dx = abs(x - PIXEL_BUFFER_WIDTH / 2);
int32_t dy = abs(y - PIXEL_BUFFER_HEIGHT / 2);
```

With a display resolution of 320 × 240 pixels, the center used by the program is $(160,120)$.

The function then determines the radii for the current frame:

```c
int32_t a = animatedRadius(RADIUS, tick_count);
int32_t b = RADIUS;
```

The horizontal radius $a$ changes according to `tick_count`, while the vertical radius $b$ remains fixed at `RADIUS`, which is defined as 40. This variation changes the ellipse's shape over time.

To avoid performing floating-point division for every pixel, the implementation rearranges the ellipse equation. Multiplying both sides by $a^2b^2$ gives:

```math
x^2b^2+y^2a^2=a^2b^2
```

The code calculates the corresponding integer values:

```c
int32_t aSquared = a * a;
int32_t bSquared = b * b;

int32_t value = dx * dx * bSquared + dy * dy * aSquared;
int32_t target = aSquared * bSquared;
```

The variable `value` represents the left-hand side of the rearranged equation, while `target` represents its right-hand side. For a pixel exactly on the mathematical boundary, the two values would be equal.

In practice, pixel coordinates are discrete integers, so an exact equality would not necessarily produce a continuous-looking ellipse. The implementation therefore allows a tolerance:

```c
if (abs(value - target) <= EPSILON * aSquared) {
    *pixel = 0xFFFF;
}
```

If the difference between `value` and `target` falls within the permitted tolerance, the pixel is colored white (`0xFFFF`). Otherwise, `centeredEllipse()` leaves the pixel unchanged. This is important because the function draws the ellipse's boundary rather than filling its interior or clearing the background.

#### Putting the Animation Together

The equation-based animation is triggered by push button 0 in either switch mode. In `main()`, the program updates `tick_count` and passes `centeredEllipse` to `animate()`:

```c
tick_count = (tick_count % PERIOD) + 1;
animate(centeredEllipse, tick_count, delta_time);
tick_count++;
```

The call `animate(centeredEllipse, tick_count, delta_time)` passes the function itself as an argument. The `animate()` function then invokes `centeredEllipse()` for each pixel, using the same tick count throughout that frame.

After the animation call returns, `main()` increments `tick_count` again and redraws the background. In the first switch mode, it calls `draw(clear)` to produce a black background. In the second mode, it calls `draw(grid)` to restore the grid pattern.

The radius calculation in `animatedRadius()` uses a period of 32 phases. As the phase progresses, the horizontal radius decreases from its maximum value toward 1 and then increases again, producing a repeating animation.

One consequence of this design is that the entire pixel buffer is processed for every frame, even though only pixels close to the ellipse's boundary are colored. This makes the implementation straightforward and allows the background to be rendered independently, but it also contributes to the slower animation observed in CPUlator.

Overall, this approach demonstrates how function pointers separate the mechanics of iterating over a pixel buffer from the specific algorithm used to draw a shape. The same mechanism can support different drawing operations simply by passing a different function to `draw()` or `animate()`.

**Challenges and approach**

My approach to the equation-based ellipse drawing was based on prior knowledge from my mathematics courses, where I had worked with the equation of a circle. I wanted to extend this idea to ellipses, so I researched the general equation of an ellipse and explored how it could be applied to drawing shapes on a pixel display.

One of the first challenges was translating the equation into a form that could be evaluated efficiently for every pixel. The original equation involves floating-point division, which would introduce unnecessary computational overhead when processing the entire pixel buffer. Through further research into how ellipse equations are used in computer graphics, I discovered that the equation could be rearranged to avoid division. This allowed the implementation to use integer arithmetic for the pixel calculations.

The second challenge was determining which pixels should be considered part of the ellipse's boundary. Initially, I checked whether the equation evaluated to an exact match, but this resulted in only a few pixels being drawn. Since the display operates on discrete pixel coordinates, many pixels lie close to the mathematical boundary without satisfying an exact equality. Introducing an epsilon value established an acceptance range around the boundary, allowing more pixels to be included and producing a more continuous ellipse.

This tolerance also introduced a way to control the apparent stroke thickness of the ellipse. By adjusting the acceptance range, the number of pixels classified as part of the boundary can be changed. Working through these challenges helped me understand how a continuous mathematical equation can be translated into a discrete graphical representation, while also revealing the performance limitations of evaluating every pixel for each animation frame.
