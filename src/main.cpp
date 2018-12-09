#include <iostream>
#include <cstdlib>
#include <array>
#include <list>
#include <random>
#include <cstdint>
#include <string>
#include <map>

#include "SDL2/SDL.h"

namespace globals {
    std::int64_t constexpr max_wait_time_ms{ 160 };
    
    int constexpr field_width{ 10 };
    int constexpr field_height{ 10 };
}

struct position_t
{
    int i{ 0 };
    int j{ 0 };

    position_t() noexcept = default;
    position_t(int const t_i, int const t_j) noexcept
        : i(t_i)
        , j(t_j)
    {}
    ~position_t() noexcept = default;
};

struct event_t
{
private:
    SDL_Scancode m_last_pressed_key{ SDL_SCANCODE_G };
    bool m_quit{ false };

    std::map<SDL_Scancode, bool> m_keys_held;
 
public:
    event_t() = default;
    ~event_t() noexcept = default;

    void poll_events()
    { 
        SDL_Event event;

        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_QUIT:
                    m_quit = true;
                    break;
                case SDL_KEYDOWN:
                    m_last_pressed_key = event.key.keysym.scancode;
                    m_keys_held[event.key.keysym.scancode] = true;
                case SDL_KEYUP:
                    m_keys_held[event.key.keysym.scancode] = false;
                default: 
                    break;
            }
        }
    } 
    constexpr char get_key() const
    { return m_last_pressed_key; }
    constexpr bool quit() const
    { return m_quit; }

    bool is_key_held(SDL_Scancode t_key) 
    {
        if(m_keys_held.find(t_key) == m_keys_held.end()) {
            return false;
        }

        return m_keys_held[t_key];
    }
};

struct window_t
{
private:
    SDL_Window* m_window{ nullptr };
    SDL_Renderer* m_renderer{ nullptr };

    int m_width{ 900 };
    int m_height{ 900 };

public:
    window_t() noexcept = delete;
    window_t(int const t_width, int const t_height) noexcept;
    ~window_t() noexcept;

    void clear_screen();
    void draw(position_t const& t_pos,
              int const t_r, int const t_g,
              int const t_b, int const t_a);
    inline void update()
    { SDL_RenderPresent(m_renderer); }

    inline void set_title(std::string const& t_title)
    { SDL_SetWindowTitle(m_window, t_title.c_str()); }
};

window_t::window_t(int const t_width, int const t_height) noexcept 
    : m_width{ t_width }
    , m_height{ t_height }
{
    SDL_Init(SDL_INIT_VIDEO);

    m_window = SDL_CreateWindow(
        "Snake Game!", 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        m_width, m_height,
        SDL_WINDOW_SHOWN
    );

    SDL_SetWindowResizable(m_window, SDL_FALSE);

    m_renderer = SDL_CreateRenderer(
        m_window, -1, SDL_RENDERER_ACCELERATED //| SDL_RENDERER_PRESENTVSYNC
    );
}

window_t::~window_t() noexcept
{
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);

    SDL_Quit();
}

void window_t::clear_screen()
{
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);
}

void window_t::draw(position_t const& t_pos,
                    int const t_r, int const t_g, 
                    int const t_b, int const t_a)
{
    SDL_Rect rect;

    rect.x = t_pos.j * (m_width / globals::field_width);
    rect.y = t_pos.i * (m_height / globals::field_height);
    rect.h = m_height / globals::field_height;
    rect.w = m_width / globals::field_width;

    SDL_SetRenderDrawColor(m_renderer, t_r, t_g, t_b, t_a);
    SDL_RenderFillRect(m_renderer, &rect);
}

struct game_field_t
{
public:
    static int constexpr width{ globals::field_width };
    static int constexpr height{ globals::field_height };

    enum cell_type
    {
        EMPTY = 0,
        SNAKE_HEAD,
        SNAKE_BODY,
        FRUIT,
        ERROR
    };

private:
    template<typename T, int Rows, int Cols>
    using matrix = std::array<std::array<T, Cols>, Rows>;

    matrix<cell_type, height, width> m_field;

    inline void clear_screen()
    { std::system("clear"); }

public:
    game_field_t() noexcept;
    ~game_field_t() noexcept = default;

    constexpr cell_type operator()(int const t_i, int const t_j) const
    { return m_field[t_i][t_j]; }
    cell_type& operator()(int const t_i, int const t_j) 
    { return m_field[t_i][t_j]; }

    cell_type at(int const t_i, int const t_j) const
    {
        if(t_i >= width || t_i < 0 || t_j >= height || t_j < 0) {
            return ERROR; 
        } 

        return this->operator()(t_i, t_j);
    }

    void draw(window_t& t_window)
    {
        t_window.clear_screen();

        int i{ 0 };
        for(auto const& line : m_field) {
            int j{ 0 };
            for(auto const cell : line) {
                switch(cell) {
                    case cell_type::SNAKE_HEAD: 
                        t_window.draw(
                            { i, j }, 
                            34, 120, 16, 255
                        );
                        break;
                    case cell_type::SNAKE_BODY: 
                        t_window.draw(
                            { i, j },
                            34, 232, 16, 255
                        );
                        break;
                    case cell_type::FRUIT: 
                        t_window.draw(
                            { i, j },
                            244, 13, 45, 255
                        );
                        break;
                    default: break;
                }  
                ++j;
            }
            ++i;
        }
    }
};

game_field_t::game_field_t() noexcept 
{
    for(auto& line : m_field) {
        line.fill(cell_type::EMPTY);
    }
}

struct snake_t
{
private:
    std::list<position_t> m_snake_positions;

    game_field_t& m_field;

    void update_field()
    {
        auto it = m_snake_positions.begin();

        m_field((*it).i, (*it).j) = game_field_t::cell_type::SNAKE_HEAD;
        ++it;

        for(auto snake_cell = it; 
            snake_cell != m_snake_positions.end(); 
            ++snake_cell)
        {
            m_field((*snake_cell).i, (*snake_cell).j) = 
                game_field_t::cell_type::SNAKE_BODY;
        }
    }

    void pop_back_snake_body()
    {
        position_t pos = m_snake_positions.back();
        m_field(pos.i, pos.j) = game_field_t::cell_type::EMPTY;
        m_snake_positions.pop_back();
    }

    inline bool is_space_for_snake(position_t const& t_pos) {
        return m_field(t_pos.i, t_pos.j) == game_field_t::cell_type::EMPTY ||
               m_field(t_pos.i, t_pos.j) == game_field_t::cell_type::FRUIT;
    }

public:
    snake_t() = delete; 
    snake_t(game_field_t& t_field)
        : m_field{ t_field }
    {
        m_snake_positions.emplace_back(
            game_field_t::height / 2 - 1,
            game_field_t::width / 2 - 1
        );

        this->update_field();
    }
    ~snake_t() noexcept = default;

    inline std::size_t get_length() const
    { return m_snake_positions.size(); }
    inline position_t get_head_position() const
    { return m_snake_positions.front(); }

    void lengthen_snake_up()
    {
        position_t head_pos = m_snake_positions.front();
        
        if(head_pos.i <= 0) {
            throw "Can't move snake up";
        }
        if(!this->is_space_for_snake({ head_pos.i - 1, head_pos.j })) {
            throw "Can't move snake up";
        }

        m_snake_positions.emplace_front(
            head_pos.i - 1, head_pos.j
        );

        this->update_field();
    }
    void move_up()
    {
        this->lengthen_snake_up();
        this->pop_back_snake_body();
    }
    
    void lengthen_snake_down()
    {
        position_t head_pos = m_snake_positions.front();
        
        if(head_pos.i >= game_field_t::height - 1) {
            throw "Can't move snake down";
        }
        if(!this->is_space_for_snake({ head_pos.i + 1, head_pos.j })) {
            throw "Can't move snake down";
        }

        m_snake_positions.emplace_front(
            head_pos.i + 1, head_pos.j
        );

        this->update_field();
    }
    void move_down()
    {
        this->lengthen_snake_down();
        this->pop_back_snake_body();
    }
    
    void lengthen_snake_left()
    {
        position_t head_pos = m_snake_positions.front();
        
        if(head_pos.j <= 0) {
            throw "Can't move snake left";
        }
        if(!this->is_space_for_snake({ head_pos.i, head_pos.j - 1 })) {
            throw "Can't move snake left";
        }

        m_snake_positions.emplace_front(
            head_pos.i, head_pos.j - 1
        );

        this->update_field();
    }
    void move_left()
    {
        this->lengthen_snake_left();
        this->pop_back_snake_body();
    }
    
    void lengthen_snake_right()
    {
        position_t head_pos = m_snake_positions.front();
        
        if(head_pos.j >= game_field_t::width - 1) {
            throw "Can't move snake right";
        }
        if(!this->is_space_for_snake({ head_pos.i, head_pos.j + 1})) {
            throw "Can't move snake right";
        }

        m_snake_positions.emplace_front(
            head_pos.i, head_pos.j + 1
        );

        this->update_field();
    }
    void move_right()
    {
        this->lengthen_snake_right();
        this->pop_back_snake_body();
    }
};

struct fruit_t
{
private:
    position_t m_position;

    game_field_t& m_field;

    position_t gen_new_position()
    {
        std::mt19937 rng;
        rng.seed(std::random_device{}());
        std::uniform_int_distribution<int> dist{ 0, game_field_t::height - 1 };

        position_t pos{ dist(rng), dist(rng) };

        while(m_field(pos.i, pos.j) != game_field_t::cell_type::EMPTY) {
            pos.i = dist(rng);
            pos.j = dist(rng);
        }

        return pos;
    }

    void update_field()
    { m_field(m_position.i, m_position.j) = game_field_t::cell_type::FRUIT; }

public:
    fruit_t() noexcept = delete;
    fruit_t(game_field_t& t_field)
        : m_field(t_field)
    { 
        m_position = this->gen_new_position(); 
        this->update_field();
    }
    ~fruit_t() noexcept = default;

    void new_position()
    {
        //position_t prev_pos = m_position;
        m_position = this->gen_new_position();
        this->update_field();
        //m_field(prev_pos.i, prev_pos.j) = game_field_t::cell_type::EMPTY;
    }

    position_t get_position() const
    { return m_position; }
};

struct game_logic_t
{
private:
    bool m_game_running{ true };
    int m_score{ 0 };

    enum direction_type
    {
        UP = 0,
        DOWN,
        LEFT, 
        RIGHT
    };

    direction_type m_direction{ UP };

    bool is_fruit_in_direction(game_field_t& t_field,
                               snake_t& t_snake)
    {
        position_t pos = t_snake.get_head_position();

        switch(m_direction) {
            case UP: --pos.i; break;
            case DOWN: ++pos.i; break;
            case LEFT: --pos.j; break;
            case RIGHT: ++pos.j; break;
            default: break;
        }

        return t_field(pos.i, pos.j) == game_field_t::cell_type::FRUIT;
    }

    void handle_movement(game_field_t& t_field, snake_t& t_snake, 
            fruit_t& t_fruit)
    {
        switch(m_direction) {
            case UP:
                if(this->is_fruit_in_direction(t_field, t_snake)) {
                    try {
                        t_snake.lengthen_snake_up();
                        t_fruit.new_position();
                    }
                    catch(...) {
                        m_game_running = false;
                    }
                }
                else {
                    try {
                        t_snake.move_up();
                    }
                    catch(...) {
                        m_game_running = false;
                    }
                }
                break;
            case DOWN:
                if(this->is_fruit_in_direction(t_field, t_snake)) {
                    try {
                        t_snake.lengthen_snake_down();
                        t_fruit.new_position();
                    }
                    catch(...) {
                        m_game_running = false;
                    }
                }
                else {
                    try {
                        t_snake.move_down();
                    }
                    catch(...) {
                        m_game_running = false;
                    }
                }
                break;
            case LEFT:
                if(this->is_fruit_in_direction(t_field, t_snake)) {
                    try {
                        t_snake.lengthen_snake_left();
                        t_fruit.new_position();
                    }
                    catch(...) {
                        m_game_running = false;
                    }
                }
                else {
                    try {
                        t_snake.move_left();
                    }
                    catch(...) {
                        m_game_running = false;
                    }
                }
                break;
            case RIGHT:
                if(this->is_fruit_in_direction(t_field, t_snake)) {
                    try {
                        t_snake.lengthen_snake_right();
                        t_fruit.new_position();
                    }
                    catch(...) {
                        m_game_running = false;
                    }
                }
                else {
                    try {
                        t_snake.move_right();
                    }
                    catch(...) {
                        m_game_running = false;
                    }
                }
                break;
            default: break;
        }
    }

public:
    game_logic_t() noexcept = default;
    ~game_logic_t() noexcept = default;

    void game_loop();

    constexpr int get_score() const
    { return m_score; }
};

void game_logic_t::game_loop()
{
    window_t window{ 900, 900 };
    game_field_t field{};
    snake_t snake{ field };
    fruit_t fruit{ field };
    event_t event{};

    std::int64_t last_time{ SDL_GetTicks() };
    std::int64_t waited_time{ 0 };

    while(m_game_running && !event.quit()) {
        std::int64_t current_time{ SDL_GetTicks() };
        std::int64_t elapsed_time = current_time - last_time;
        last_time = current_time;
        waited_time += elapsed_time;

        window.set_title(
            std::string{ "Snake Game! Score: " } + 
            std::to_string(snake.get_length())
        );

        field.draw(window);
        window.update();
        
        event.poll_events();

        if(waited_time >= globals::max_wait_time_ms) {
            switch(event.get_key()) {
                case SDL_SCANCODE_UP:
                    if(m_direction == DOWN) break;
                    m_direction = UP;
                    break;
                case SDL_SCANCODE_LEFT:
                    if(m_direction == RIGHT) break;
                    m_direction = LEFT; 
                    break;
                case SDL_SCANCODE_DOWN: 
                    if(m_direction == UP) break;
                    m_direction = DOWN;
                    break;
                case SDL_SCANCODE_RIGHT: 
                    if(m_direction == LEFT) break;
                    m_direction = RIGHT;
                    break;
                case SDL_SCANCODE_ESCAPE: 
                    m_game_running = false; 
                    break;
                default: break;
            } 

            this->handle_movement(field, snake, fruit);
            waited_time = 0;
        }
    }

    m_score = snake.get_length();
}

int main()
{
    bool volatile replay{ true };

    while(replay) {
        game_logic_t game;
        game.game_loop();
        
        std::cout << "Score: " << game.get_score() << std::endl;
        std::cout << "Replay? [y/n]" << std::endl;

        char ch;
        std::cin >> ch;
        if(ch == 'y' || ch == 'Y') {
            replay = true;
        }
        else {
            replay = false;
        }
    }
}


