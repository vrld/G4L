Name pending
------------

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

    local gl = require 'OpenGLua'
    
    gl.initMode(gl.mode.rgba, gl.mode.double)
    gl.clearColor(.05,.1,.3,.1)
    
    local win = gl.newWindow("OpenGLua", 800,600)
    
    function win.reshape(w,h)
        gl.viewport(0,0, w,h)
    end
    
    local frames = 0
    function win.update(dt)
        frames = frames + 1
    end
    
    function win.draw()
        gl.clear()
        win:swap()
    end
    
    gl.timer(.25, function()
        win:title('OpenGLua -- FPS: ' .. (frames * 4))
        frames = 0
        return .25
    end)
    
    gl.run()

## API

### Window management/stuff

    gl.initMode(mode, ...)
    win = gl.newWindow(title, w,h, x,y)
    gl.run()
    gl.timer(delay, function)

### State machine

    gl.enable(flag)
    gl.disable(flag)
    status = gl.isEnabled(flag)
    gl.clearColor(vec4)
    gl.clearColor(r,g,b, [a])
    gl.clearDepth([depth])
    gl.clearStencil([s])
    gl.blendFunc(src, dst, [srcAlpha, dstAlpha])
    gl.blendEquation(equation)
    gl.stencilFunc(func, [ref, mask])
    gl.stencilOp(sfail, [dpfail, dppass])

### Drawing

    gl.viewport(x,y, w,h)
    gl.clear([buffer-bit, ...])

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

    vec2 = gl.math.vec(x,y)
    vec3 = gl.math.vec(x,y,z)
    vec4 = gl.math.vec(x,y,z,w)

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

    mat22 = gl.math.mat(a,b, c,d)
    mat33 = gl.math.mat(a,b,c, d,e,f, g,h,i)
    mat44 = gl.math.mat(a,b,c,d, e,f,g,h, i,j,k,l, m,n,o,p)

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

Corresponding to the GL\_ and GLUT\_ counterparts.

### gl.flags

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

### gl.mode

    rgb
    rgba
    index
    single
    double
    alpha
    depth
    stencil
    multisample
    stereo
    luminance

### gl.cursor

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

### gl.buffer

    stream_draw
    stream_read
    stream_copy
    static_draw
    static_read
    static_copy
    dynamic_draw
    dynamic_read
    dynamic_copy

### gl.blend

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


### gl.buffer_bit

    color
    depth
    stencil

### gl.stencil
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
