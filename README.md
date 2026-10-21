# 02 - Displays

<br>

## Overview

This experiment explores the mathematical principles behind drawing and animating ellipses in the CPUlator ARMv7 DE1-SoC System simulator.

The initial approach uses the equation of an ellipse to determine which pixels lie along its boundary. By varying one of the ellipse's radii over time, the program creates an animation in which the ellipse continuously changes its shape. This demonstrates how a mathematical equation can be translated into pixel-level graphics and used to produce dynamic visual output.

The experiment also explores Bresenham-style rasterization algorithms as an alternative approach to drawing ellipses. Comparing these methods provides an opportunity to examine different ways of translating geometric shapes into discrete pixels on a display.

### Table of contents

1. [Objectives](#objectives)
2. [Control Scheme](#control-scheme)
3. [Implementation Overview](#implementation)
    * 3.1 [VGA Buffer](#vga-pixel-buffer)
    * 3.2 [Approaches to Ellipse Drawing](#approaches-to-ellipse-drawing)

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

<br>

### 5. Bresenham-Based Ellipse Animation

Unlike the equation-based approach, which evaluates every pixel in the display buffer, the Bresenham-based approach calculates points along the ellipse's boundary and plots them directly. This reduces the number of pixels that need to be processed and makes it more suitable for drawing animated shapes.

The implementation is divided into several functions. `ellipseBres()` calculates the ellipse boundary, `drawEllipsePoints()` plots the calculated points, and `drawBresLine()` connects selected points to the centre of the display. The animation functions then combine these operations to produce moving ellipses.

#### 5.1. Calculating the Ellipse Boundary

The main function responsible for drawing an ellipse is `ellipseBres()`. It uses a Bresenham-style, midpoint-based algorithm to determine which pixel coordinates best approximate the mathematical ellipse.

Rather than evaluating the ellipse equation at every coordinate on the screen, the algorithm works through the ellipse's boundary and determines which pixels should be plotted as it progresses. The implementation divides the calculation into two regions to account for the different slopes along the ellipse. This allows the boundary to be traced using incremental calculations instead of checking the entire display.

The horizontal radius is also animated using `animatedRadius()`, while the vertical radius remains fixed. This allows the ellipse to change shape over time, following the same shrinking and growing pattern used by the equation-based approach.

#### 5.2. Plotting and Rotating Ellipse Points

Once the boundary points have been calculated, `drawEllipsePoints()` is responsible for plotting them. The function uses the calculated coordinates together with precomputed sine and cosine values to transform the points before drawing them with `putPixel()`.

To avoid repeatedly calculating trigonometric functions during animation, the implementation stores sine and cosine values in two lookup tables, `sinTable` and `cosTable`. Each table contains 32 entries, scaled by a factor of 1024 to represent the values using fixed-point integer arithmetic.

The angle is represented by an index into these tables. By changing the index, the implementation can select different sine and cosine values and alter the orientation of the plotted ellipse. The fixed-point representation allows these transformations to be performed using integer arithmetic rather than floating-point calculations.

The `drawEllipsePoints()` function also calls `drawBresLine()` for selected points when either coordinate is zero. This draws lines between the centre and those points, adding lines to the rendered shape when the relevant condition is met.

#### 5.3. Drawing Lines with Bresenham's Algorithm

The `drawBresLine()` function uses Bresenham's line algorithm to draw a straight line between two coordinates. Instead of calculating every point using floating-point operations, the algorithm uses integer-based error calculations to determine which pixels best approximate the line.

The function plots the line by calling `putPixel()` for the calculated coordinates. Since `putPixel()` checks whether a coordinate falls within the display boundaries before writing to the pixel buffer, the drawing operation respects the limits of the screen.

This function is also used by `drawEllipsePoints()` to connect selected ellipse points to the centre. Separating line drawing from ellipse calculation allows the same line-drawing routine to be reused wherever a straight line is needed.

#### 5.4. Animating and Erasing Ellipses

The implementation contains three animation functions that use the Bresenham-based drawing routines:

- `animateEllipseBres()` draws a single white ellipse.
- `animateMultEllipsesBres()` draws two ellipses using the colours `0x00FF` and `0xF00F`.
- `animateMultTraceEllipsesBres()` draws two ellipses while updating the angle indices passed to the drawing routines.

Each function draws its ellipse or ellipses, calls `delay()` to control the animation timing, and then draws over the shapes using black (`0x0000`) to erase them. The functions subsequently increment `tick_count` and return its updated value.

The multiple-ellipse functions also use a relative angle alongside the main angle index. This allows different angle values to be supplied to the two ellipses, while the trace variant updates the angle arguments between drawing and erasing operations.

The animation functions therefore combine boundary calculation, point plotting, timing and erasure into a single sequence. The main loop can call the appropriate function in response to a push-button input without needing to perform each drawing step itself.

#### 5.5. Background Handling

The Bresenham-based animations erase shapes by drawing them again in black. This is straightforward when the background is entirely black, because the erased pixels return to the background colour.

However, this approach does not restore the original colour of a pixel. In the second switch mode, where the grid is used as the background, drawing black over an ellipse can replace grid pixels with black. The grid is therefore not automatically preserved when these animations run.

Overall, the Bresenham-based implementation demonstrates an alternative way to generate ellipse graphics. By calculating and plotting boundary points directly, it avoids scanning the entire pixel buffer for every frame and provides a foundation for animating multiple ellipses with different colours and angle indices.

 **Note on direction selection**
 ```c
sx = (x0 < x1) ? 1 : -1;
sy = (y0 < y1) ? 1 : -1;
```

 These statements determine the direction in which the line-drawing algorithm moves along the horizontal and vertical axes. The conditional operator (`?:`) checks whether the starting coordinate is smaller than the ending coordinate. If it is, the corresponding step is set to `1`, moving in the positive direction; otherwise, it is set to `-1`, moving in the negative direction.
 
The variables `sx` and `sy` therefore allow the same line-drawing algorithm to handle lines in different directions, including lines that slope upwards, downwards, left or right. Instead of requiring separate algorithms for different line orientations, the step values determine how the coordinates are updated as the algorithm progresses from the starting point to the endpoint.

This differs from the equation-based approach, where the background can be redrawn after the animation. In the current Bresenham implementation, erasure relies on the assumption that the background behind the animated shapes is black. Preserving an existing background would require a different approach to restoring the pixels covered by each ellipse.

<br>

**Challenges and approach**

Coming from the equation-based approach, efficiency and performance were the main motivations for exploring an alternative method of drawing ellipses. Since evaluating every pixel in the display buffer for each frame was computationally expensive, I searched online for more efficient drawing techniques and came across Bresenham's algorithms. These offered a way to draw shapes by calculating the boundary pixels directly instead of processing the entire screen.

Although implementing the algorithms was relatively straightforward once I had found suitable references, understanding how they actually worked was a much greater challenge. Rather than simply copying an implementation, I wanted to understand the reasoning behind each calculation and how the algorithm decided which pixel to draw next.

Over time, I realised that the decision parameters were central to this process. They determine how the algorithm evaluates its progress along a line or ellipse and chooses the next pixel to approximate the intended boundary. Describing this process in general terms or writing simplified pseudocode was relatively intuitive, but translating that understanding into a complete implementation required a much deeper grasp of what happens after each pixel is selected. In particular, I had to understand how the decision parameters are updated at each step and how the symmetry of an ellipse allows its boundary to be divided into regions and calculated efficiently.

Another idea that influenced the implementation was the approach to erasing animated shapes. I came across a YouTube video discussing how older graphics and animations had to work within strict memory and performance constraints. One technique involved modifying selected parts of an existing image to create the illusion of movement, rather than repeatedly redrawing the entire scene. This inspired me to erase the ellipses by drawing over them in black before drawing the next frame. Although this is a simple solution for a black background, it also helped me recognise the importance of considering how an animation interacts with the image behind it.

Overall, exploring Bresenham's algorithms helped me move beyond treating drawing algorithms as ready-made solutions. It gave me a better understanding of how decision parameters, pixel selection and geometric symmetry work together to make raster graphics more efficient. It also showed me that improving performance involves more than choosing a faster drawing algorithm: the way an animation is erased and the background is maintained can affect the overall design as well.

<br>

## Notes and Limitations

The current implementation demonstrates two approaches to drawing and animating ellipses, but several aspects of the code limit its flexibility and affect how it can be used.

- **Background handling:** The Bresenham-based animation functions erase ellipses by drawing over them in black (`0x0000`). This works on a black background, but it does not restore pixels that previously belonged to the grid or another shape. Supporting arbitrary backgrounds would require storing and restoring the original pixel colours, or redrawing the background and all relevant shapes after each frame.

- **Performance trade-offs:** The equation-based approach evaluates every pixel in the 320 × 240 display buffer for each frame, making it more computationally expensive than plotting only the boundary pixels. The Bresenham-based approach reduces the number of pixels that need to be processed, but its current erasure method limits how well it works with more complex backgrounds. Therefore, drawing efficiency is only one consideration when designing an animation system.

- **Fixed display configuration:** The implementation uses a fixed pixel-buffer base address (`0xC8000000`), display dimensions of 320 × 240 pixels, and a row stride of 1024 bytes. These assumptions are specific to the current display configuration and would need to be revisited if the program were adapted to a different resolution or pixel-buffer layout.

- **Limited animation parameters:** The ellipse is positioned around the fixed display centre `(160, 120)`, and its dimensions are determined by constants such as `RADIUS` and the fixed vertical radius. The animation changes the horizontal radius over time, but the current implementation does not provide general controls for changing the ellipse's position, dimensions or other properties independently.

- **Discrete angle resolution:** The sine and cosine lookup tables contain 32 entries, so the available angle indices are limited to a fixed set of values. This keeps the transformations simple and avoids calculating trigonometric functions repeatedly, but it also restricts the range of orientations that can be represented directly by the lookup tables.

- **Blocking animation timing:** The animation functions use a busy-wait delay to control the time between drawing and erasing shapes. This occupies the processor while the delay runs and does not provide precise timing guarantees. A more advanced implementation could use a timer or another timing mechanism to control animation updates.

- **Restricted input handling:** The program recognises specific switch states and exact push-button register values. Only the combinations explicitly handled in the main loop trigger their corresponding actions. For example, when both switches are on, neither display-mode branch runs, and unrecognised button combinations do not trigger an animation. Expanding the input handling would allow more flexible interaction.

- **Pixel restoration and overlapping shapes:** The multiple-ellipse animations draw and erase shapes in sequence without maintaining a separate representation of the underlying scene. If shapes overlap, erasing one can also overwrite pixels belonging to another. A more general graphics implementation would need to account for overlapping objects and preserve the correct background when redrawing them.

Despite these limitations, the current implementation provides a useful comparison between equation-based and Bresenham-based rasterization. It demonstrates how the choice of drawing algorithm affects performance, and how animation timing, pixel restoration and background management influence the overall behaviour of a graphics program.

<br>

## Learning Outcomes

- **Applying Mathematical Knowledge:** I learned how to build on my existing knowledge of circle equations and extend it to ellipses. Translating the equation into a working drawing algorithm also taught me that understanding the mathematics is only part of the process, as it must be adapted to the limitations of pixel-based graphics.

- **Understanding Algorithms Beyond Their Implementation:** Working with Bresenham's algorithms taught me the importance of understanding the reasoning behind an algorithm rather than simply implementing an existing solution. In particular, exploring the decision parameters helped me understand how individual pixel choices approximate a continuous geometric shape.

- **Recognising the Importance of Efficiency:** Comparing the two approaches gave me a practical understanding of how algorithm design affects performance. I could see why evaluating every pixel becomes expensive and how calculating only the relevant boundary points provides a more efficient alternative.

- **Working with Low-Level Graphics:** Implementing the drawing routines gave me experience working directly with memory addresses, pointer arithmetic and the pixel buffer. It also helped me understand how mathematical calculations ultimately translate into changes on a simulated display.

- **Understanding Animation and Its Limitations:** Developing the animation routines taught me that drawing a shape is only one part of creating an animation. I also had to consider timing, erasing previous frames and preserving the background. This helped me recognise how seemingly simple implementation choices can affect the behaviour and flexibility of the overall program.

Overall, this experiment helped me connect mathematical concepts with practical algorithm design and low-level programming. More importantly, it taught me to look beyond whether an implementation works and to understand why it works, what makes it efficient, and which limitations arise from the choices made during development.

<br>

## Possible Extensions

- **Background Preservation:** The Bresenham-based animations could be extended to restore the original pixel colours instead of erasing shapes by drawing over them in black. This would allow the animations to work with different backgrounds and make it easier to combine multiple graphical elements.

- **Interactive Shape Manipulation:** Additional controls could be introduced to adjust the position, size and orientation of the ellipses. This would allow users to explore how changes to the underlying parameters affect the resulting shapes in real time.

- **Additional Drawing Algorithms:** Other rasterization algorithms could be implemented and compared with the existing equation-based and Bresenham-based approaches. This would provide further insight into their differences in performance, accuracy and computational complexity.

- **More Complex Animations:** The existing drawing routines could be expanded to support more complex animations involving multiple independently moving or rotating shapes. This would also introduce the challenge of managing overlapping objects and preserving the background between frames.

---

*This experiment is part of Dev-Trace, a collection documenting my ongoing learning process with C programming and the CPUlator ARMv7 DE1-SoC System.*
