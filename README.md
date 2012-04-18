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


## Possible Applications

Prototyping/visualizations/live coding/awesome


## Example

    local g4l = require 'G4L'
    
    g4l.initMode(g4l.screen.rgba, g4l.screen.double)
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
    g4l.elapsed()

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

### Buffer Objects

    buffer = g4l.vbo([record-size, target, usage, initial_size])
    buffer = buffer:push(record, [...]) -- record = {a,b,c,...} with #record = record-size OR record = number if record-size == 1
    buffer = buffer:clear()
    buffer = buffer:finish()
    buffer:draw(draw_mode)

### Framebuffer Objects

    fbo = g4l.framebuffer(width, height, [disable-renderbuffer = false])
    texture = fbo:addAttachment([texture-unit = 1])
    complete, error-message = fbo:isComplete()
    g4l.setFramebuffer([fbo = nil])

### Shader

    shader = g4l.shader(vertex_source, fragment_source)
    shader:warnings()

#### Uniform access

    shader.<uniform> = (number|vector|matrix)
    table = shader.<uniform>

#### Vertex Attributes

    shader:enableAttribute(name, [...])
    shader:disableAttribute(name, [...])
    shader:bindAttribute(name, buffer, [min, max, normalize])

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

    vec2 = g4l.math.vector(x,y)
    vec3 = g4l.math.vector(x,y,z)
    vec4 = g4l.math.vector(x,y,z,w)

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
    mat = vec:diag()

#### Matrices

    mat22 = g4l.math.matrix(a,b, c,d)
    mat33 = g4l.math.matrix(a,b,c, d,e,f, g,h,i)
    mat44 = g4l.math.matrix(a,b,c,d, e,f,g,h, i,j,k,l, m,n,o,p)

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

#### Predefined Matrices

##### Model transformation

    g4l.math.model.unit([s])
    g4l.math.model.rotate(axis, phi)
    g4l.math.model.scale(sx, [sy, sz])
    g4l.math.model.translate(vec)

##### View transformation

    g4l.math.view.lookAt(eye, center, up)

##### Projection

    g4l.math.project.ortho(left, right, bottom, top, [near, far])
    g4l.math.project.perspective(fovy, aspect, [near, far])
    g4l.math.project.frustum(left, right, bottom, top, near, far)

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

### g4l.screen

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


### g4l.clear_bit

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

### g4l.texture_flags

    clamp_to_edge
    clamp_to_border
    mirrored_repeat
    repeat
    
    nearest
    linear

### g4l.draw_mode

    points
    lines
    line_strip
    line_loop
    triangles
    triangle_stop
    triangle_fan
    
    lines_adjacency
    line_strip_adjacency
    triangles_adjacency
    triangle_strip_adjacency
