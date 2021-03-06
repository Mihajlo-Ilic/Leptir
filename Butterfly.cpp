#include "Butterfly.hpp"

using namespace bfly;

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!GLOBAL STATES AND VARIABLES!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

int component_counter = 0;

GLuint vertices_id;
GLuint edges_id;
GLuint program_id;

GLuint check_full;

GLuint orto_location;
GLuint model_location;
GLuint type_location;
GLuint color1_location;
GLuint color2_location;
GLuint opacity_location;
GLuint rect_size_location;
GLuint border_width_location;
GLuint z_location;

bool mouse_hold = false;
bool mouse_click = false;
bool mouse_hold_begin = false;
bool mouse_hold_end = false;

int mouse_state = GLFW_RELEASE;

int key_pressed = -1;

Style panel_style;
Font regular_font;

int mouse_x = 0;
int mouse_y = 0;

int delta_x=0;
int delta_y=0;

int window_height = 0;

pair<Drawable *, float> focused_element = make_pair(nullptr, -1.0f);
pair<Drawable *, float> current_focused_element = make_pair(nullptr, -1.0f);

//!!!!!!!!!!!!!!!!!!!!
//!!HELPER FUNCTIONS!!
//!!!!!!!!!!!!!!!!!!!!

void focus(Drawable *d)
{
    if (d != nullptr)
    {
        focused_element = make_pair(d, 0);
    }
}

void clear_focus()
{
    if (focused_element.first != nullptr)
    {
        focused_element.first->on_focus_lost();
    }
    focused_element = make_pair(nullptr, -1.0f);
}

void update_focus(pair<Drawable *, float> p)
{
    if (current_focused_element.second <= p.second)
        current_focused_element = make_pair(p.first, p.second);
}

void compute_focus()
{
    if (current_focused_element.first != nullptr)
    {
        current_focused_element.first->on_focus();
        current_focused_element = make_pair(nullptr, -1.0f);
    }
    else
    {
        if (mouse_click)
            clear_focus();
    }
}

void load_projection_matrix(float left, float right, float up, float down)
{
    float znear = -1.0f;
    float zfar = 1.0f;
    float mat[4][4] = {
        {2.0f / (right - left), 0, 0, -(right + left) / (right - left)},
        {0, 2.0f / (up - down), 0, -(up + down) / (up - down)},
        {0, 0, -2.0f / (zfar - znear), -(zfar + znear) / (zfar - znear)},
        {0, 0, 0, 1.0f}};
    glUniformMatrix4fv(orto_location, 1, GL_TRUE, &(mat[0][0]));
}

void load_model_matrix(float x, float y, float width, float heigth)
{
    float mat[4][4] = {
        {width, 0, 0, x},
        {0, heigth, 0, y},
        {0, 0, 1, 0},
        {0, 0, 0, 1}};
    glUniformMatrix4fv(model_location, 1, GL_TRUE, &(mat[0][0]));
}

bool mouse_in_rect(Rectangle r)
{
    return (mouse_x >= r.x && mouse_x <= r.x + r.width && mouse_y >= r.y && mouse_y <= r.y + r.height);
}

void load_vec3(GLuint id, Vector3 v)
{
    glUniform3f(id, v.r, v.g, v.b);
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!IMAGE AND FILE LOADING!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!

GLuint load_bmp(string path)
{
    ifstream file(path, ios::binary | ios::in);
    GLuint text_id;

    if (file.is_open())
    {
        char buffer[10];
        unsigned int width;
        unsigned int heigth;
        unsigned int bit_per_pixel;
        unsigned int data_position;

        file.read(buffer, 2);
        file.read(buffer, 4);
        file.read(buffer, 2);
        file.read(buffer, 2);
        file.read(buffer, 4);

        memcpy(&data_position, buffer, sizeof(unsigned int));

        file.read(buffer, 4);
        file.read(buffer, 4);

        memcpy(&width, buffer, sizeof(unsigned int));

        file.read(buffer, 4);

        memcpy(&heigth, buffer, sizeof(unsigned int));

        file.read(buffer, 2);
        file.read(buffer, 2);

        memcpy(&bit_per_pixel, buffer, sizeof(unsigned int));
        if (bit_per_pixel != 24 && bit_per_pixel != 32)
        {
            cerr << "Bfly error when loading image data for" << path << endl;
            cerr << "image contains " << bit_per_pixel << " bits per pixel" << endl;
            cerr << "image is not 24 bits per pixel or 32 bits per pixel" << endl;
            return 0;
        }
        file.seekg(data_position);
        char pixels[width][heigth * (bit_per_pixel / 8)];
        for (int i = 0; i < heigth; i++)
            for (int j = 0; j < width; j++)
            {
                unsigned int r;
                unsigned int g;
                unsigned int b;
                unsigned int a;

                file.read(buffer, 1);
                memcpy(&b, buffer, sizeof(unsigned int));
                file.read(buffer, 1);
                memcpy(&g, buffer, sizeof(unsigned int));
                file.read(buffer, 1);
                memcpy(&r, buffer, sizeof(unsigned int));

                if (bit_per_pixel == 32)
                {
                    file.read(buffer, 1);
                    memcpy(&a, buffer, sizeof(unsigned int));

                    pixels[heigth - (i + 1)][4 * j] = g;
                    pixels[heigth - (i + 1)][4 * j + 1] = b;
                    pixels[heigth - (i + 1)][4 * j + 2] = r;
                    pixels[heigth - (i + 1)][4 * j + 3] = a;
                }
                else
                {
                    pixels[heigth - (i + 1)][3 * j] = g;
                    pixels[heigth - (i + 1)][3 * j + 1] = b;
                    pixels[heigth - (i + 1)][3 * j + 2] = r;
                }
            }

        glGenTextures(1, &text_id);
        glBindTexture(GL_TEXTURE_2D, text_id);

        glTexParameteri(GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,
                        GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,
                        GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        if (bit_per_pixel == 24)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, heigth, 0, GL_RGB, GL_UNSIGNED_BYTE, &pixels);
        else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, heigth, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels);
        file.close();
        return text_id;
    }

    cerr << "Bfly couldn't open " << path << endl;
    return 0;
}

Font load_font(string path)
{
    Font fnt;
    ifstream file(path + ".fnt");
    string line;
    while (getline(file, line))
    {
        if (!line.rfind("char id=", 0))
        {
            int x, y, w, h, xo, yo, xa;
            int c;
            sscanf(line.c_str(), "char id=%d      x=%d    y=%d    width=%d    height=%d    xoffset=%d    yoffset=%d    xadvance=%d", &c, &x, &y, &w, &h, &xo, &yo, &xa);
            fnt.characters[c] = {x, y, w, h, xo, yo, xa};
        }
        if (!line.rfind("common lineHeight", 0))
        {
            int lin;
            int base;
            int sw;
            int sh;
            sscanf(line.c_str(), "common lineHeight=%d base=%d scaleW=%d scaleH=%d", &lin, &base, &sw, &sh);
            fnt.line_height = lin;
            fnt.base = base;
            fnt.scale_w = sw;
            fnt.scale_h = sh;
        }
        if (!line.rfind("info face=", 0))
        {
            int size;
            char name[50];
            sscanf(line.c_str(), "info face=\"%s size=%d", name, &size);
            fnt.size = size;
            fnt.name = string(name);
            fnt.name.pop_back();
        }
    }
    fnt.font_atlas = load_bmp(path + ".bmp");
    return fnt;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!INPUT HANDLING SECTION!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!

void character_callback(GLFWwindow *window, unsigned int codepoint)
{
    key_pressed = codepoint;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        mouse_hold = true;
    }
    if (action == GLFW_RELEASE && mouse_state == GLFW_PRESS)
    {
        mouse_click = true;
        mouse_hold = false;
        mouse_hold_end = true;
    }
    if (action == GLFW_PRESS && mouse_state == GLFW_RELEASE)
    {
        mouse_hold_begin=true;
    }
    mouse_state = action;
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos)
{
    delta_x=(int)xpos-mouse_x;
    delta_y=(int)ypos-mouse_y;
    mouse_x = (int)xpos;
    mouse_y = (int)ypos;
}

void Butterfly_Input(GLFWwindow *window)
{
    glfwPollEvents();
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCharCallback(window, character_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
}

void Butterfly_Refresh()
{
    key_pressed = -1;
    mouse_click = false;
    mouse_hold_begin = false;
    mouse_hold_end = false;
    delta_x=0;
    delta_y=0;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!CONSTRUCTORS AND DATA INITIALIZATION!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void Styles_Initialize()
{
    regular_font = load_font("./Resources/Butterfly");
    panel_style.font = regular_font;
    panel_style.color1 = {0.32f, 0.34f, 0.32f};
    panel_style.color2 = {0.22f, 0.24f, 0.22f};
    panel_style.border_width = 1;
    panel_style.opacity = 1.0f;
    panel_style.text_color = {1.0f, 1.0f, 1.0f};
    panel_style.text_size = 25;
    panel_style.border_color = {0.9f, 0.9f, 0.9f};
}

void Butterfly_Initialize()
{
    Styles_Initialize();
    load_bmp("Resources/ikon.bmp");
    load_bmp("Resources/down.bmp");
    load_bmp("Resources/right.bmp");
    check_full = load_bmp("Resources/check.bmp");
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    ifstream file("./Resources/v_shader.vert");
    if (file.is_open())
    {
        stringstream ss;
        ss << file.rdbuf();
        string s = ss.str();
        const char *text = s.c_str();
        glShaderSource(vertex_shader, 1, &text, NULL);
        glCompileShader(vertex_shader);
        int result;
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &result);
        if (!result)
        {
            char info[512];
            glGetShaderInfoLog(vertex_shader, 512, NULL, info);
            cerr << "Bfly Error compiling vertex shader" << endl;
            cerr << info << endl;
        }
        file.close();
    }
    else
        cerr << "error opening vertex shader" << endl;

    file.open("./Resources/f_shader.frag");
    if (file.is_open())
    {
        stringstream ss;
        ss << file.rdbuf();
        string s = ss.str();
        const char *text = s.c_str();
        glShaderSource(fragment_shader, 1, &text, NULL);
        glCompileShader(fragment_shader);
        int result;
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &result);
        if (!result)
        {
            char info[512];
            glGetShaderInfoLog(fragment_shader, 512, NULL, info);
            cerr << "Bfly Error compiling fragment shader" << endl;
            cerr << info << endl;
        }
    }
    else
        cerr << "error opening vertex shader" << endl;

    program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader);
    glAttachShader(program_id, fragment_shader);
    glLinkProgram(program_id);

    int result;
    glGetProgramiv(program_id, GL_LINK_STATUS, &result);
    if (!result)
    {
        char info[512];
        glGetProgramInfoLog(program_id, 512, NULL, info);
        cerr << "Bfly Error linking shaders" << endl;
    }

    orto_location = glGetUniformLocation(program_id, "projection");
    model_location = glGetUniformLocation(program_id, "model");
    type_location = glGetUniformLocation(program_id, "type");
    color1_location = glGetUniformLocation(program_id, "color1");
    color2_location = glGetUniformLocation(program_id, "color2");
    opacity_location = glGetUniformLocation(program_id, "opacity");
    rect_size_location = glGetUniformLocation(program_id, "rect_size");
    border_width_location = glGetUniformLocation(program_id, "border_width");
    z_location = glGetUniformLocation(program_id, "z");

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    float array[] = {
        0.0, 0.0,
        1.0, 0.0,
        1.0, 1.0,

        1.0, 1.0,
        0.0, 1.0,
        0.0, 0.0};

    GLuint vbo;
    glGenVertexArrays(1, &vertices_id);
    glBindVertexArray(vertices_id);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(array), array, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glBindVertexArray(0);
}

GLuint make_text(string text, Style &style, float &text_width, float &text_height)
{
    float scale = (float)style.text_size / (float)style.font.line_height;
    GLuint vao;
    GLuint vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    vector<float> arr;
    float x_it = 0;
    float y_it = 0;
    text_width = 0;
    text_height = 0;
    for (auto it : text)
    {
        Glyph glyph = style.font.characters[it];

        float x0 = x_it + glyph.x_off * scale;
        float x1 = x0 + glyph.width * scale;
        float y0 = y_it + (glyph.y_off * scale);
        float y1 = y0 + glyph.height * scale;
        if (fabs(y1) > text_height)
            text_height = fabs(y1);
        float ux = (float)glyph.uv_x / (float)style.font.scale_w;
        float ux1 = ux + (float)glyph.width / (float)style.font.scale_w;
        float uy = (float)glyph.uv_y / style.font.scale_h;
        float uy1 = uy + (float)glyph.height / (float)style.font.scale_h;
        float ar[] = {
            x0, y0, ux, uy,
            x1, y0, ux1, uy,
            x1, y1, ux1, uy1,

            x1, y1, ux1, uy1,
            x0, y1, ux, uy1,
            x0, y0, ux, uy};
        for (auto f : ar)
            arr.push_back(f);
        x_it += glyph.x_adv * scale;
    }
    text_width = x_it;
    glBufferData(GL_ARRAY_BUFFER, arr.size() * sizeof(float), &arr[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return vao;
}

void Component::set_text(string t)
{
    if (text != t)
    {
        text = t;
        glDeleteVertexArrays(1, &vao);
        vao = make_text(text, style, text_width, text_height);
    }
}

void Component::set_text_size(int size)
{
    if (style.text_size != size)
    {
        style.text_size = size;
        glDeleteVertexArrays(1, &vao);
        vao = make_text(text, style, text_width, text_height);
    }
}

Panel::Panel() : Component("tmp_panel_" + to_string(component_counter++))
{
    rect = {50, 50, 400, 400};
    name = "tmp_panel_" + to_string(component_counter++);
    tooltip = "";
    text = "header";
    elements.clear();
    style = panel_style;
    style.color1 = {0.4f, 0.42f, 0.41f};
    style.color2 = {0.34f, 0.32f, 0.33f};
    vao = make_text(text, style, text_width, text_height);
    z=-0.5f;

    horizontal=new ScrollBar();
    horizontal->set_position(rect.x,rect.height+rect.y-20);
    horizontal->set_width(rect.width);

    vertical=new ScrollBar();
    vertical->set_position(rect.x,rect.y+26);
    vertical->set_size(20,rect.height-20);
    vertical->set_orientation(VERTICAL_SLIDER);
}

Label::Label():Component("tmp_label_" + to_string(component_counter++)){
    rect = {75, 75, 100, 35};
    tooltip = "";
    text = "label";
    style = panel_style;
    vao = make_text(text, style, text_width, text_height);
    image_id = 0;
    icon = false;
    state = IDLE;
    order=LEFT_TO_RIGHT;
    allign_h=LEFT;
    allign_v=TOP;
}

Button::Button() : Component("tmp_button_" + to_string(component_counter++))
{
    rect = {75, 75, 100, 35};
    tooltip = "";
    text = "button";
    style = panel_style;
    vao = make_text(text, style, text_width, text_height);
    image_id = 0;
    press_color = {style.color1.r + 0.1f, style.color1.g + 0.1f, style.color1.b + 0.1f};
    click_color = {style.color1.r + 0.05f, style.color1.g + 0.05f, style.color1.b + 0.05f};
    click = nullptr;
    background_rectangle = true;
    icon = false;
    state = IDLE;
    order=LEFT_TO_RIGHT;
}

void Button::rect_uniforms()
{
    glUniform1i(type_location, 0);
    if (state == IDLE)
    {
        load_vec3(color1_location, style.color1);
        load_vec3(color2_location, style.color2);
    }
    if (state == HOVER)
    {
        load_vec3(color1_location, press_color);
        load_vec3(color2_location, press_color);
    }
    if (state == PRESS)
    {
        load_vec3(color1_location, click_color);
        load_vec3(color2_location, click_color);
    }
    if (state == CLICKED)
    {
        load_vec3(color1_location, click_color);
        load_vec3(color2_location, click_color);
    }
}

void Slider::rect_uniforms(){
    glUniform1i(type_location, 0);
    if (state == IDLE)
    {
        load_vec3(color1_location, style.color1);
        load_vec3(color2_location, style.color2);
    }
    if (state == HOVER)
    {
        load_vec3(color1_location, press_color);
        load_vec3(color2_location, press_color);
    }
    if (state == PRESS)
    {
        load_vec3(color1_location, click_color);
        load_vec3(color2_location, click_color);
    }
    if (state == CLICKED)
    {
        load_vec3(color1_location, click_color);
        load_vec3(color2_location, click_color);
    }
}

ToggleButton::ToggleButton() : Component("tmp_toggle_button" + to_string(component_counter++))
{
    rect = {75, 75, 100, 35};
    tooltip = "";
    text = "toggle";
    style = panel_style;
    vao = make_text(text, style, text_width, text_height);
    image_id = 0;
    press_color = {style.color1.r + 0.1f, style.color1.g + 0.1f, style.color1.b + 0.1f};
    click_color = {style.color1.r + 0.05f, style.color1.g + 0.05f, style.color1.b + 0.05f};
    toggle = nullptr;
    background_rectangle = true;
    icon = false;
    value = false;
}

void ToggleButton::rect_uniforms()
{
    glUniform1i(type_location, 0);
    if (state == IDLE)
    {
        if (value)
        {
            load_vec3(color1_location, {0.2f, 0.30f, 0.20f});
            load_vec3(color2_location, {0.2f, 0.30f, 0.20f});
        }
        else
        {
            load_vec3(color1_location, style.color1);
            load_vec3(color2_location, style.color2);
        }
    }
    if (state == HOVER)
    {
        load_vec3(color1_location, press_color);
        load_vec3(color2_location, press_color);
    }
    if (state == PRESS)
    {
        load_vec3(color1_location, click_color);
        load_vec3(color2_location, click_color);
    }
    if (state == CLICKED)
    {
        load_vec3(color1_location, click_color);
        load_vec3(color2_location, click_color);
    }
}

CheckBox::CheckBox() : Component("tmp_checkbox" + to_string(component_counter++))
{
    rect = {75, 75, 100, 35};
    tooltip = "";
    text = "checkbox";
    style = panel_style;
    vao = make_text(text, style, text_width, text_height);
    image_id = 0;
    press_color = {style.color1.r + 0.1f, style.color1.g + 0.1f, style.color1.b + 0.1f};
    click_color = {style.color1.r + 0.05f, style.color1.g + 0.05f, style.color1.b + 0.05f};
    toggle = nullptr;
    icon = false;
    value = false;
}

RadioButton::RadioButton() : Component("tmp_radio" + to_string(component_counter++))
{
    rect = {75, 75, 100, 35};
    tooltip = "";
    text = "radio button";
    style = panel_style;
    vao = make_text(text, style, text_width, text_height);
    image_id = 0;
    press_color = {style.color1.r + 0.1f, style.color1.g + 0.1f, style.color1.b + 0.1f};
    click_color = {style.color1.r + 0.05f, style.color1.g + 0.05f, style.color1.b + 0.05f};
    toggle = nullptr;
    icon = false;
    value = false;
    
}

ComboItem::ComboItem() : Component("tmp_combo_item" + to_string(component_counter++))
{
    rect = {75, 75, 175, 25};
    tooltip = "";
    text = "combo item";
    style = panel_style;
    vao = make_text(text, style, text_width, text_height);
    image_id = 0;
    press_color = {style.color1.r + 0.1f, style.color1.g + 0.1f, style.color1.b + 0.1f};
    click_color = {style.color1.r + 0.05f, style.color1.g + 0.05f, style.color1.b + 0.05f};
    opened = false;
    sub_item_width = 175;
    sub_item_height = 25;
}

ComboBox::ComboBox() : Component("tmp_combo_box" + to_string(component_counter++))
{
    rect = {75, 75, 175, 25};
    tooltip = "";
    text = "combo box";
    style = panel_style;
    style.text_size = rect.height - 1;
    vao = make_text(text, style, text_width, text_height);
    image_id = 0;
    press_color = {style.color1.r + 0.1f, style.color1.g + 0.1f, style.color1.b + 0.1f};
    click_color = {style.color1.r + 0.05f, style.color1.g + 0.05f, style.color1.b + 0.05f};
    opened = false;
    item_width = 175;
    item_height = 25;
    selected = nullptr;
    opened = false;
    change = nullptr;
}



void ComboBox::set_default_text(string t)
{
    glDeleteVertexArrays(1, &default_vao);
    default_vao = make_text(t, style, text_width, text_height);
}

void ComboBox::rect_uniforms()
{
    glUniform1i(type_location, 0);
    if (state == IDLE)
    {
        load_vec3(color1_location, style.color1);
        load_vec3(color2_location, style.color2);
    }
    if (state == HOVER)
    {
        load_vec3(color1_location, press_color);
        load_vec3(color2_location, press_color);
    }
    if (state == PRESS)
    {
        load_vec3(color1_location, click_color);
        load_vec3(color2_location, click_color);
    }
    if (state == CLICKED)
    {
        load_vec3(color1_location, click_color);
        load_vec3(color2_location, click_color);
    }
}

void ComboBox::change_selected(ComboItem *item)
{
    if (item != nullptr && item != selected)
    {
        glDeleteVertexArrays(1, &vao);
        selected = item;
        vao = make_text(selected->get_text(), style, text_width, text_height);
        image_id=item->get_image();
        if (change != nullptr)
            change();
    }
}

void ComboItem::rect_uniforms()
{
    glUniform1i(type_location, 0);
    if (state == IDLE)
    {
        load_vec3(color1_location, style.color1);
        load_vec3(color2_location, style.color2);
    }
    if (state == HOVER)
    {
        load_vec3(color1_location, press_color);
        load_vec3(color2_location, press_color);
    }
    if (state == PRESS)
    {
        load_vec3(color1_location, click_color);
        load_vec3(color2_location, click_color);
    }
    if (state == CLICKED)
    {
        load_vec3(color1_location, click_color);
        load_vec3(color2_location, click_color);
    }
}



Slider::Slider():Component("tmp_slider_" + to_string(component_counter++)){
    minimum=0;
    maximum=100;
    value=0;
    step=1;
    orientation=HORIZONTAL_SLIDER;

    rect = {75, 75, 175, 25};
    tooltip = "";
    text = "0";
    style = panel_style;
    style.text_size = rect.height - 1;
    vao = make_text(text, style, text_width, text_height);
    rect.width+=text_width;
    image_id = 0;
    state = IDLE;
    slider_size=10;
}

ScrollBar::ScrollBar():Component("tmp_scrollbar" + to_string(component_counter++)){
    maximum=0;
    hidden=true;
    slider_position=0;
    orientation=HORIZONTAL_SLIDER;
    rect = {75, 75, 100, 20};
    style = panel_style;
    style.text_size = 0;
    vao = 0;
    image_id = 0;
    state = IDLE;
    press_color = {style.color1.r + 0.1f, style.color1.g + 0.1f, style.color1.b + 0.1f};
    click_color = {style.color1.r + 0.05f, style.color1.g + 0.05f, style.color1.b + 0.05f};
}

void ScrollBar::rect_uniforms()
{
    glUniform1i(type_location, 0);
    if (state == IDLE)
    {
        load_vec3(color1_location, style.color1);
        load_vec3(color2_location, style.color2);
    }
    if (state == HOVER)
    {
        load_vec3(color1_location, press_color);
        load_vec3(color2_location, press_color);
    }
    if (state == PRESS)
    {
        load_vec3(color1_location, click_color);
        load_vec3(color2_location, click_color);
    }
    if (state == CLICKED)
    {
        load_vec3(color1_location, click_color);
        load_vec3(color2_location, click_color);
    }
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!DRAWING METHODS!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void draw_rectangle(Rectangle r)
{
    load_model_matrix((float)r.x, (float)r.y, (float)r.width, (float)r.height);
    glBindVertexArray(vertices_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void draw_text(GLuint vao, int x, int y, int size)
{
    load_model_matrix((float)x, (float)y, 1, 1);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, size);
    glBindVertexArray(0);
}

void Panel::prepare_style()
{
    glUniform3f(color1_location, style.color1.r, style.color1.g, style.color1.b);
    glUniform3f(color2_location, style.color2.r, style.color2.g, style.color2.b);
    glUniform1f(opacity_location, style.opacity);
    glUniform1i(type_location, 0);
}

void Panel::prepare_style_text()
{
    glUniform3f(color1_location, style.text_color.r, style.text_color.g, style.text_color.b);
    glUniform1f(opacity_location, style.opacity);
    glBindTexture(GL_TEXTURE_2D, style.font.font_atlas);
    glUniform1i(type_location, 1);
}

void Panel::prepare_style_header()
{
    glUniform3f(color1_location, style.color1.r, style.color1.g, style.color1.b);
    glUniform3f(color2_location, style.color2.r, style.color2.g, style.color2.b);
    glUniform1f(opacity_location, style.opacity);
    glUniform1i(type_location, 0);
}

void Panel::draw()
{
    glUniform1i(border_width_location, style.border_width);
    glUniform2f(rect_size_location, rect.width, rect.height);
    glUniform1f(opacity_location, style.opacity);
    glUniform1f(z_location, z);
    prepare_style();
    draw_rectangle(rect);
    prepare_style_header();
    glUniform2f(rect_size_location, rect.width, style.text_size + 2);
    draw_rectangle({rect.x, rect.y, rect.width, style.text_size + 2});
    prepare_style_text();
    draw_text(vao, rect.x + 5, rect.y, text.size() * 12);

    float box[4];
    glGetFloatv(GL_SCISSOR_BOX, box);
    glScissor(rect.x, window_height - rect.y - rect.height, rect.width, rect.height);

    for (auto it : elements){
        int x_curr=it->get_x();
        int y_curr=it->get_y();
        it->set_x(it->get_x()-horizontal->get_value());
        it->set_y(it->get_y()-vertical->get_value());
        it->draw();
        it->set_x(x_curr);
        it->set_y(y_curr);
    }

    horizontal->draw();
    vertical->draw();

    glScissor(box[0], box[1], box[2], box[3]);
}

void Button::draw()
{
    
    glUniform1i(border_width_location, style.border_width);
    glUniform2f(rect_size_location, rect.width, rect.height);
    glUniform1f(z_location, z);
    process_events();
    float box[4];
    glGetFloatv(GL_SCISSOR_BOX, box);
    glScissor(max(rect.x,(int)box[0]), window_height - rect.y - rect.height, rect.width, rect.height);
    if (background_rectangle)
    {
        rect_uniforms();
        draw_rectangle(rect);
    }
    int x_pos = 0;
    int y_pos = 0;
    x_pos = rect.width - text_width;
    if (icon)
        x_pos -= text_height;
    y_pos = rect.height - text_height;

    x_pos = max(0, x_pos / 2);
    y_pos = max(0, y_pos / 2);

    if (!icon)
    {
        glBindTexture(GL_TEXTURE_2D, image_id);
        glUniform1i(type_location, 2);
        if (image_id != 0)
            draw_rectangle({rect.x + 1, rect.y + 1, rect.width - 1, rect.height - 1});
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, image_id);
        glUniform1i(type_location, 2);
        int ord = 0;
        if (order == LEFT_TO_RIGHT)
            ord = text_width;
        if (image_id != 0)
            draw_rectangle({rect.x + x_pos + ord, rect.y + y_pos, (int)text_height, (int)text_height});
    }
    load_vec3(color1_location, style.text_color);
    glBindTexture(GL_TEXTURE_2D, style.font.font_atlas);
    glUniform1i(type_location, 1);
    int ord = 0;
    if (order == RIGHT_TO_LEFT)
        ord = +text_height;
    draw_text(vao, rect.x + x_pos + ord, rect.y + y_pos, text.size() * 12);
    glScissor(box[0], box[1], box[2], box[3]);
}

void Label::draw(){
    glUniform1f(opacity_location, style.opacity);
    glUniform1f(z_location, z);
    process_events();
    float box[4];
    glGetFloatv(GL_SCISSOR_BOX, box);
    glScissor(max(rect.x,(int)box[0]), window_height - rect.y - rect.height, rect.width, rect.height);
    if(icon==false){
        glBindTexture(GL_TEXTURE_2D,image_id);
        glUniform1i(type_location,2);
        if(image_id!=0)
        draw_rectangle(rect);
    }
    float width=text_width;

    if(icon==true)
        width+=4+text_height;

    float x_pos=0;
    float y_pos=0;

    if(allign_h==LEFT) x_pos=rect.x;
    if(allign_h==CENTER) {
        x_pos=rect.width-width;
        x_pos=max(0.0f,x_pos/2.0f);
        x_pos=rect.x+x_pos;
    }
    if(allign_h==RIGHT) x_pos=rect.x+rect.width-width;

    if(allign_v==TOP) y_pos=rect.y;
    if(allign_v==CENTER) {
        y_pos=rect.height-text_height;
        y_pos=max(0.0f,y_pos/2.0f);
        y_pos=rect.y+y_pos;
    }
    if(allign_v==BOTTOM) y_pos=rect.y+rect.height-text_height;


    glBindTexture(GL_TEXTURE_2D,style.font.font_atlas);
    glUniform1i(type_location,1);
    load_vec3(color1_location,style.text_color);
    draw_text(vao,x_pos,y_pos,text.size()*12);

    if(icon){
    glBindTexture(GL_TEXTURE_2D,image_id);
    glUniform1i(type_location,2);
    if(image_id!=0)
        draw_rectangle({(int)(x_pos+text_width),(int)y_pos,(int)text_height,(int)text_height});
    }
    glScissor(box[0], box[1], box[2], box[3]);
}

void ToggleButton::draw()
{
    
    glUniform1i(border_width_location, style.border_width);
    glUniform2f(rect_size_location, rect.width, rect.height);
    glUniform1f(z_location, z);
    process_events();
    float box[4];
    glGetFloatv(GL_SCISSOR_BOX, box);
    glScissor(max(rect.x,(int)box[0]), window_height - rect.y - rect.height, rect.width, rect.height);
    if (background_rectangle)
    {
        rect_uniforms();
        draw_rectangle(rect);
    }
    int x_pos = 0;
    int y_pos = 0;
    x_pos = rect.width - text_width;
    if (icon)
        x_pos -= text_height;
    y_pos = rect.height - text_height;

    x_pos = max(0, x_pos / 2);
    y_pos = max(0, y_pos / 2);

    if (!icon)
    {
        glBindTexture(GL_TEXTURE_2D, image_id);
        glUniform1i(type_location, 2);
        if (image_id != 0)
            draw_rectangle({rect.x + 1, rect.y + 1, rect.width - 1, rect.height - 1});
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, image_id);
        glUniform1i(type_location, 2);
        int ord = 0;
        if (order == LEFT_TO_RIGHT)
            ord = text_width;
        if (image_id != 0)
            draw_rectangle({rect.x + x_pos + ord, rect.y + y_pos, (int)text_height, (int)text_height});
    }
    load_vec3(color1_location, style.text_color);
    glBindTexture(GL_TEXTURE_2D, style.font.font_atlas);
    glUniform1i(type_location, 1);
    int ord = 0;
    if (order == RIGHT_TO_LEFT)
        ord = +text_height;
    draw_text(vao, rect.x + x_pos + ord, rect.y + y_pos, text.size() * 12);
    glScissor(box[0], box[1], box[2], box[3]);
}

void CheckBox::draw()
{
    
    glUniform1i(border_width_location, style.border_width);
    glUniform2f(rect_size_location, text_height, text_height);
    glUniform1f(z_location, z);
    process_events();
    glUniform1i(type_location, 1);
    load_vec3(color1_location, style.text_color);
    glBindTexture(GL_TEXTURE_2D, style.font.font_atlas);
    draw_text(vao, rect.x, rect.y, text.size() * 12);

    glUniform1i(type_location, 0);
    glBindTexture(GL_TEXTURE_2D, check_full);
    load_vec3(color1_location, style.color1);
    load_vec3(color2_location, style.color2);
    draw_rectangle({rect.x + (int)text_width + 5, rect.y + 2, (int)text_height, (int)text_height});

    if (value)
    {
        glUniform1i(type_location, 2);
        draw_rectangle({rect.x + (int)text_width + 5, rect.y + 2, (int)text_height, (int)text_height});
    }
}

void RadioButton::draw()
{
    
    glUniform1i(border_width_location, style.border_width);
    glUniform2f(rect_size_location, text_height, text_height);
    glUniform1f(z_location, z);
    process_events();
    glUniform1i(type_location, 1);
    load_vec3(color1_location, style.text_color);
    glBindTexture(GL_TEXTURE_2D, style.font.font_atlas);
    draw_text(vao, rect.x, rect.y, text.size() * 12);

    glUniform1i(type_location, 3);
    load_vec3(color1_location, style.color1);
    load_vec3(color2_location, style.color2);
    draw_rectangle({rect.x + (int)text_width + 5, rect.y + 2, (int)text_height, (int)text_height});

    if (value)
    {
        load_vec3(color1_location, {0.2f, 0.3f, 0.2f});
        draw_rectangle({rect.x + (int)text_width + 5, rect.y + 2, (int)text_height, (int)text_height});
    }
}

void ComboItem::draw()
{
    
    glUniform1i(border_width_location, 0);
    glUniform2f(rect_size_location, text_height, text_height);
    glUniform1f(z_location, 1);
    process_events();
    rect_uniforms();
    if(state==HOVER || state==PRESS || state==CLICKED)
    draw_rectangle({rect.x+1,rect.y+1,rect.width-1,rect.height-1});
    glUniform1i(type_location, 1);
    load_vec3(color1_location, style.text_color);
    glBindTexture(GL_TEXTURE_2D, style.font.font_atlas);
    draw_text(vao, rect.x + text_height + 5, rect.y, text.size() * 12);

    if (image_id != 0)
    {
        glUniform1i(type_location, 2);
        glBindTexture(GL_TEXTURE_2D, image_id);
        draw_rectangle({rect.x, rect.y+2, (int)text_height, (int)text_height});
    }
    if (sub_items.size() >= 1)
    {
        glUniform1i(type_location, 2);
        glBindTexture(GL_TEXTURE_2D, 4);
        draw_rectangle({rect.x + rect.width - rect.height, rect.y+5, (int)text_height, (int)text_height});
        if (opened)
        {
            int x = rect.x + rect.width;
            int y = rect.y;
            glUniform2f(rect_size_location, rect.width, (int)sub_items.size() * sub_item_height);
            glUniform1i(type_location, 0);
            load_vec3(color1_location, style.color1);
            load_vec3(color2_location, style.color2);
            glUniform1i(border_width_location, 1);
            draw_rectangle({x, rect.y, sub_item_width, (int)sub_items.size() * sub_item_height});
            for (auto it : sub_items)
            {
                it->set_position(x, y);
                it->draw();
                y += sub_item_height;
            }
        }
    }
}

void ComboBox::draw()
{
    
    glUniform1i(border_width_location, style.border_width);
    glUniform2f(rect_size_location, rect.width, rect.height);
    glUniform1f(z_location, z);
    process_events();
    rect_uniforms();
    draw_rectangle(rect);
    if(image_id!=0){
        glUniform1i(type_location, 2);
        glBindTexture(GL_TEXTURE_2D, image_id);
        draw_rectangle({rect.x+5,rect.y+2,(int)text_height,(int)text_height});
    }
    glUniform1i(type_location, 1);
    load_vec3(color1_location, style.text_color);
    glBindTexture(GL_TEXTURE_2D, style.font.font_atlas);
    draw_text(vao, rect.x + 10+text_height, rect.y, text.size() * 12);

    glUniform1i(type_location, 2);
    glBindTexture(GL_TEXTURE_2D, 3);
    draw_rectangle({rect.x + rect.width - rect.height - 5, rect.y, rect.height, rect.height});
    float x = rect.x;
    float y = rect.y + rect.height;
    if (opened)
    {
        glUniform2f(rect_size_location, rect.width, (int)items.size() * item_height );
        glUniform1i(type_location, 0);
        load_vec3(color1_location, style.color1);
        load_vec3(color2_location, style.color2);
        glUniform1f(z_location, 1.0f);
        draw_rectangle({rect.x, rect.y + rect.height, item_width, (int)items.size() * item_height});
        for (auto it : items)
        {
            it->set_z(1.0f);
            it->set_position(x, y);
            it->draw();
            y += item_height;
        }
    }
}

void Slider::draw(){
    glUniform1i(border_width_location, 0);
    glUniform2f(rect_size_location, rect.width, rect.height);
    glUniform1f(z_location, 0);

    process_events();

    load_vec3(color1_location,style.color1);
    load_vec3(color2_location,style.color2);
    glUniform1i(type_location,0);

    draw_rectangle({rect.x,rect.y+rect.height/2-2,rect.width,4});
    rect_uniforms();

    float scale=value/(maximum-minimum);
    int x_pos=(int)(scale*((float)rect.width-text_height));

    glUniform1i(type_location,3);
    glUniform1i(border_width_location, 1);
    glUniform2f(rect_size_location, rect.height, rect.height);
    draw_rectangle({rect.x+x_pos,rect.y+2,(int)text_height,(int)text_height});


    load_vec3(color1_location,style.text_color);
    glBindTexture(GL_TEXTURE_2D,style.font.font_atlas);
    glUniform1i(type_location,1);
    draw_text(vao,rect.x+rect.width+5,rect.y,text.size()*12);

}

void ScrollBar::draw(){
    glUniform1i(border_width_location, 0);
    glUniform2f(rect_size_location, rect.width, rect.height);
    glUniform1i(border_width_location, 1);
    glUniform1f(z_location, 0);

    process_events();

    glUniform3f(color1_location+0.1, style.color1.r+0.1, style.color1.g+0.1, style.color1.b+0.1);
    glUniform3f(color2_location+0.1, style.color2.r+0.1, style.color2.g+0.1, style.color2.b+0.1);
    glUniform1i(type_location,0);

    draw_rectangle(rect);

    rect_uniforms();
    
    if(orientation==HORIZONTAL_SLIDER){
        glUniform2f(rect_size_location, slider_size, rect.height);
        draw_rectangle({rect.x+slider_position,rect.y,slider_size,rect.height});
    }
    else{
        glUniform2f(rect_size_location, rect.width, slider_size);
        draw_rectangle({rect.x,rect.y+slider_position,rect.width,slider_size});
    }
}


void Window::draw(GLFWwindow *window)
{

    glUseProgram(program_id);
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    load_projection_matrix(0, w, 0, h);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL); 
    glScissor(0, 0, w, h);
    window_height = h;
    glViewport(0, 0, w, h);

    for (auto panel : panels)
        panel->draw();

    compute_focus();

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(0);

}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!EVENT PROCESSING METHODS!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void Button::process_events()
{
    if (mouse_in_rect(rect))
    {
        update_focus(make_pair(this, z));
    }
    else
        state = IDLE;
}

void Label::process_events(){
    if (mouse_in_rect(rect))
    {
        update_focus(make_pair(this, z));
    }
}

void ToggleButton::process_events()
{
    int box[4];
    glGetIntegerv(GL_SCISSOR_BOX, box);
    int real_x=max(box[0],rect.x);
    int real_width=min(box[0]+box[2]-rect.x,rect.width);
    Rectangle real_rect=rect;
    real_rect.x=real_x;
    real_rect.width=real_width;
    int real_y=max(box[0],rect.y);
    int real_height=min(box[1]+box[3]-rect.y,rect.height);
    real_rect.y=real_y;
    real_rect.height=real_height;
    if(mouse_in_rect(real_rect))
    {
        update_focus(make_pair(this, z));
    }
    else
        state = IDLE;
}

void CheckBox::process_events()
{
    if (mouse_in_rect({rect.x + (int)text_width + 5, rect.y + 2, (int)text_height, (int)text_height}))
    {
        update_focus(make_pair(this, z));
    }
    else
        state = IDLE;
}

void RadioButton::process_events()
{
    if (mouse_in_rect({rect.x + (int)text_width + 5, rect.y + 2, (int)text_height, (int)text_height}))
    {
        update_focus(make_pair(this, z));
    }
    else
        state = IDLE;
}

void ComboItem::process_events()
{
    if (mouse_in_rect(rect))
    {
        update_focus(make_pair(this, z));
    }
    else
        state = IDLE;
}

void ComboBox::process_events()
{
    if (mouse_in_rect(rect))
    {
        update_focus(make_pair(this, z));
    }else
        state = IDLE;
}

void Slider::process_events()
{
    float scale=value/(maximum-minimum);
    int x_pos=(int)(scale*((float)rect.width-text_height));
    
    if (mouse_in_rect({rect.x+x_pos,rect.y+2,(int)text_height,(int)text_height}) && mouse_hold_begin)
    {
        update_focus(make_pair(this, z));
    }else
        state = IDLE;
    if(focused_element.first!=nullptr && focused_element.first==this){
        int x=mouse_x-rect.x;
        x=max(0,x);
        x=min(rect.width-(int)text_height,x);
        float scale=(float)x/((float)rect.width-text_height);
        value=scale*maximum;
        glDeleteVertexArrays(1,&vao);
        stringstream ss;
        ss<<fixed<<setprecision(0)<<value;
        text=ss.str();
        vao=make_text(ss.str(),style,text_width,text_height);
    }
}

void ScrollBar::process_events()
{
    if(orientation==HORIZONTAL_SLIDER){
    if (mouse_in_rect({rect.x+slider_position,rect.y,slider_size,rect.height}))
    {
        state = HOVER;
        if(mouse_hold_begin){
            relative_pos=mouse_x;
            update_focus(make_pair(this, z));
        }
    }else {
        state = IDLE;
        
    }
    if(focused_element.first!=nullptr && focused_element.first==this){
        state = PRESS;
        int x=delta_x;
        slider_position+=x;
        slider_position=max(slider_position,0);
        slider_position=min(slider_position,rect.width-slider_size);
        }
    }

    if(orientation==VERTICAL_SLIDER){
    if (mouse_in_rect({rect.x,rect.y+slider_position,rect.width,slider_size}))
    {
        state = HOVER;
        if(mouse_hold_begin){
            relative_pos=mouse_x;
            update_focus(make_pair(this, z));
        }
    }else {
        state = IDLE;
        
    }
    if(focused_element.first!=nullptr && focused_element.first==this){
        state = PRESS;
        int x=delta_y;
        slider_position+=x;
        slider_position=max(slider_position,0);
        slider_position=min(slider_position,rect.height-slider_size);
    }
    }
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!FOCUS PROCESSING METHODS!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void Button::on_focus()
{
    if (mouse_click)
    {
        clear_focus();
        state = CLICKED;
        if (click != nullptr)
            click();
    }
    else if (mouse_hold)
        state = PRESS;
    else
        state = HOVER;
}

void Button::on_focus_lost()
{
    state = IDLE;
}

void Label::on_focus()
{
    if (mouse_click)
    {
        clear_focus();

    }

}

void Label::on_focus_lost()
{
    
}

void ToggleButton::on_focus()
{
    if (mouse_click)
    {
        clear_focus();
        state = CLICKED;
        value = !value;
        if (toggle != nullptr)
            toggle();
    }
    else if (mouse_hold)
        state = PRESS;
    else
        state = HOVER;
}

void ToggleButton::on_focus_lost()
{
    state = IDLE;
}

void CheckBox::on_focus()
{
    if (mouse_click)
    {
        clear_focus();
        state = CLICKED;
        value = !value;
        if (toggle != nullptr)
            toggle();
    }
    else if (mouse_hold)
        state = PRESS;
    else
        state = HOVER;
}

void CheckBox::on_focus_lost()
{
    state = IDLE;
}

void RadioButton::on_focus()
{
    if (mouse_click)
    {
        clear_focus();
        state = CLICKED;

        if (rg->multiple == false)
        {
            if (rg->allow_none == false)
            {
                if (rg->selected == nullptr)
                {
                    value = true;
                    rg->selected = this;
                }
                else if(rg->selected!=this)
                {
                    value = true;
                    rg->selected->value = false;
                    rg->selected = this;
                }
            }
            else
            {
                if (rg->selected != nullptr && rg->selected != this)
                    rg->selected->value = false;
                value = !value;
                rg->selected = this;
            }
        }
        else
        {
            if (rg->allow_none)
            {
                value = !value;
                if (value)
                    rg->num_selected++;
                else
                    rg->num_selected--;
            }
            else
            {
                if (value && rg->num_selected == 1)
                {
                }
                else
                {
                    value = !value;
                    if (value)
                        rg->num_selected++;
                    else
                        rg->num_selected--;
                }
            }
        }
    }
    else if (mouse_hold)
        state = PRESS;
    else
        state = HOVER;
}

void RadioButton::on_focus_lost()
{
    state = IDLE;
}

void ComboBox::on_focus()
{
    if (mouse_click)
    {
        opened = !opened;
        clear_focus();
        state = CLICKED;
        if (opened)
        {
            focus(this);
        }
    }
    else if (mouse_hold)
        state = PRESS;
    else
        state = HOVER;
}

void ComboBox::on_focus_lost()
{
    state = IDLE;
    opened = false;
    for (auto it : items)
        it->close();
}

void ComboItem::on_focus()
{
    if (mouse_click)
    {
        if (sub_items.size() > 0)
        {
            opened = !opened;
            if (opened==true)
            {
                clear_focus();
                state = CLICKED;
                focus(this);
                ComboItem *curr=this;
                while(curr!=nullptr){
                    curr->opened=true;
                    curr=curr->parent_item;
                }
                parent_box->set_open(true);
            } 
        }
        else
        {
            clear_focus();
            parent_box->change_selected(this);
        }
    }
    else if (mouse_hold)
        state = PRESS;
    else
        state = HOVER;
}

void ComboItem::on_focus_lost()
{
    state = IDLE;
    ComboItem *curr = this;
    while (curr != nullptr)
    {
        curr->opened = false;
        curr = curr->parent_item;
    }

    parent_box->set_open(false);
}


void Slider::on_focus()
{
        if(mouse_hold){
            clear_focus();
            focus(this);
        } 
        else state=IDLE;
}

void Slider::on_focus_lost()
{
    state=IDLE;
}

void ScrollBar::on_focus()
{
        if(mouse_hold){
            clear_focus();
            focus(this);
        } 
        else state=IDLE;
}

void ScrollBar::on_focus_lost()
{
    state=IDLE;
}

void Slider::update_value(int x){
    x=x-rect.x;
    x=max(0,x);
    x=min(rect.width,x);
    float scale=(float)x/(float)rect.width;
    value=scale*maximum;
    glDeleteVertexArrays(1,&vao);
    stringstream ss;
    ss<<fixed<<setprecision(8)<<value;
    text=ss.str();
    vao=make_text(ss.str(),style,text_width,text_height);
}
