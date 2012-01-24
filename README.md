G4L - Graphics for Lua
----------------------

Modern OpenGL wrapper/graphics engine for Lua.

## Why?

The goal is to provide access to OpenGL 3.3, including:

- Window creation with basic input (using GLUT)
- Shader creation and communication
- Vector and matrix math support
- Vertex and Index Buffer Objects
- Easy texture loading (libjpeg, libpng)

... and all that as a lua library, not as a standalone program.


## Possible Applications

Prototyping/visualizations/live coding/awesome

## Example

    local g4l = require 'G4L'
    
    g4l.initMode(g4l.mode.rgba, g4l.mode.double)
    g4l.clearColor(.05,.1,.3,.1)
    
    local win = g4l.newWindow("G4L", 800,600)
    
    function win.reshape(w,h)
        g4l.viewport(0,0, w,h)
    end
    
    local frames = 0
    function win.update(dt)
        frames = frames + 1
    end
    
    function win.draw()
        g4l.clear()
        win:swap()
    end
    
    g4l.timer(.25, function()
        win:title('G4L -- FPS: ' .. (frames * 4))
        frames = 0
        return .25
    end)
    
    g4l.run()

## API

### Window management/stuff

    g4l.initMode(mode, ...)
    win = g4l.newWindow(title, w,h, x,y)
    g4l.run()
    g4l.timer(delay, function)

### State machine

    g4l.enable(flag)
    g4l.disable(flag)
    status = g4l.isEnabled(flag)
    g4l.clearColor(vec4)
    g4l.clearColor(r,g,b, [a])
    g4l.clearDepth([depth])
    g4l.clearStencil([s])
    g4l.blendFunc(src, dst, [srcAlpha, dstAlpha])
    g4l.blendEquation(equation)
    g4l.stencilFunc(func, [ref, mask])
    g4l.stencilOp(sfail, [dpfail, dppass])

### Drawing

    g4l.viewport(x,y, w,h)
    g4l.clear([buffer-bit, ...])

### Window

#### Functions

    win:destroy()
    win:redraw()
    win:swap()
    win:position(x,y)
    win:resize(w,h)
    win:fullscreen()
    win:show()
    win:hide()
    win:iconify()
    win:title(title)
    win:cursor(cursor)

#### Callbacks

    win.draw()
    win.reshape(w,h)
    win.update(dt)
    win.visibility(visible)
    win.keyboard(key, x,y)
    win.mousereleased(x,y, button)
    win.mousepressed(x,y, button)
    win.mousedraw(x,y)
    win.mousemove(x,y)
    win.mouseleave()
    win.mouseenter()

### Math

#### Vectors

    vec2 = g4l.math.vec(x,y)
    vec3 = g4l.math.vec(x,y,z)
    vec4 = g4l.math.vec(x,y,z,w)

##### element access

    vec.[xyzw]

##### arithmetic

    -vec
    vec + vec
    vec - vec
    vec * scalar
    scalar * vec
    vec / scalar
    vec * vec        ... dot product
    vec ^ vec        ... det(v1,v2) [vec2], cross product [vec3], undefined [vec4]
    vec .. vec       ... per-element multiplication

##### relation

    #vec             ... number of elements
    vec == vec       ... all elements equal
    vec <  vec       ... all elements less
    vec <= vec       ... lexicographical sort

##### other

    vec = vec:clone()
    vec = vec:map(function)     ... has side effects!
    x,y,z,w = vec:unpack()
    vec = vec:reset(x,y,[z,w])  ... has side effects!
    length = vec:len()
    length^2 = vec:len2()
    dist = vec:dist(vec)
    vec = vec:permul(vec)
    vec = vec:normalized()
    vec = vec:normalize_inplace() ... has side effects!
    vec = vec:projectOn(vec)
    vec = vec:mirrorOn(vec)

#### Matrices

    mat22 = g4l.math.mat(a,b, c,d)
    mat33 = g4l.math.mat(a,b,c, d,e,f, g,h,i)
    mat44 = g4l.math.mat(a,b,c,d, e,f,g,h, i,j,k,l, m,n,o,p)

##### element access

    mat[i][k]       ... 1 <= i <= rows, 1 <= k <= cols
    mat(i,k)        ... 1 <= i <= rows, 1 <= k <= cols
    mat:get(i,k)
    mat:set(i,k, val)

##### arithmetic

    -mat
    mat + mat
    mat - mat
    scalar * mat
    mat * scalar
    mat / sclar
    mat * vec
    mat * mat
    mat .. mat    ... per element multiplication

##### relation

    #mat          ... number of rows
    mat == mat    ... all elements equal

##### other

    mat = mat:clone()
    mat = mat:map()           ... has side effects!
    a,b,... = mat:unpack()
    mat = mat:reset(a,b,...)  ... has side effects!
    mat = mat:permul(mat)
    s = mat:det()
    mat = mat:transposed()
    mat = mat:transpose_inplace() ... has side effects!
    vec = mat:trace()

## Enums

Corresponding to the g4l\_ and g4lUT\_ counterparts.

### g4l.flags

    blend
    color_logic_op
    cull_face
    depth_clamp
    depth_test
    dither
    line_smooth
    multisample
    polygon_offset_fill
    polygon_offset_line
    polygon_offset_point
    polygon_smooth
    primitive_restart
    sample_alpha_to_coverage
    sample_alpha_to_one
    sample_coverage
    scissor_test
    stencil_test
    texture_cube_map_seamless
    program_point_size

### g4l.mode

    rgb
    rgba
    index
    sing4le
    double
    alpha
    depth
    stencil
    multisample
    stereo
    luminance

### g4l.cursor

    right_arrow
    left_arrow
    left_info
    left_destroy
    help
    cycle
    spray
    wait
    text
    crosshair
    up_down
    left_right
    top_side
    bottom_side
    left_side
    right_side
    top_left_corner
    top_right_corner
    bottom_right_corner
    bottom_left_corner
    full_crosshair
    none
    inherit

### g4l.buffer

    stream_draw
    stream_read
    stream_copy
    static_draw
    static_read
    static_copy
    dynamic_draw
    dynamic_read
    dynamic_copy

### g4l.blend

    zero
    one
    src_color
    one_minus_src_color
    dst_color
    one_minus_dst_color
    src_alpha
    one_minus_src_alpha
    dst_alpha
    one_minus_dst_alpha
    constant_color
    one_minus_constant_color
    constant_alpha
    one_minus_constant_alpha
    src_alpha_saturate

    add
    subtract
    reverse_subtract
    min
    max


### g4l.buffer_bit

    color
    depth
    stencil

### g4l.stencil
    never
    less
    lequal
    greater
    gequal
    equal
    notequal
    always
    
    keep
    zero
    replace
    incr
    incr_wrap
    decr
    decr_wrap
    invert
