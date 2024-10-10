const float speed = 100.0f;
const float screen_width = 320.0f;
const float screen_height = 180.0f;
const float paddle_padding = 20.0f;
const float spin_factor = 1.5f;
const float ai_range = 15.0f;

Gfx_Font *
    font;
u32 font_height = 48;

typedef enum EntityArchetype
{
    arch_nil = 0,
    arch_player = 1,
    arch_ball = 2,
    arch_enemy = 3,
    ARCH_MAX,

} EntityArchetype;

typedef struct Entity
{
    bool is_valid;
    EntityArchetype arch;
    Vector2 position;
    Vector2 velocity;
    Vector2 size;
    Vector4 color;
    int score;
} Entity;

#define MAX_ENTITY_COUNT 1024

typedef enum UXState
{
    UX_nil,
    UX_main_menu,
    UX_pause,
    UX_game_over,
    UX_MAX,
} UXState;

typedef struct World
{
    Entity entities[MAX_ENTITY_COUNT];
    UXState ux_state;
} World;

World *world = 0;

Entity *entity_create()
{
    Entity *entity_found = 0;
    for (int i = 0; i < MAX_ENTITY_COUNT; i++)
    {
        Entity *existing_entity = &world->entities[i];
        if (!existing_entity->is_valid)
        {
            entity_found = existing_entity;
            break;
        }
    }
    assert(entity_found, "No more free entities!");
    entity_found->is_valid = true;
    return entity_found;
}

void entity_destory(Entity *entity)
{
    memset(entity, 0, sizeof(Entity));
}

void setup_player(Entity *en)
{
    en->arch = arch_player;
    en->position = v2(-(screen_width / 2) + paddle_padding, 0);
    en->size = v2(10, 40);
    en->color = COLOR_WHITE;
    en->score = 0;
}

void setup_ball(Entity *en)
{
    en->arch = arch_ball;
    en->position = v2(0, 0);
    en->velocity = v2(-1, 1);
    en->size = v2(5, 5);
    en->color = COLOR_WHITE;
}

void setup_enemy(Entity *en)
{
    en->arch = arch_enemy;
    en->size = v2(10, 40);
    en->position = v2((screen_width / 2) - paddle_padding, 0);
    en->color = COLOR_WHITE;
    en->score = 0;
}

bool is_colliding(Entity *a, Entity *b)
{
    bool colliding = (a->position.x - (a->size.x / 2) < b->position.x + (b->size.x / 2) &&
                      a->position.x + (a->size.x / 2) > b->position.x - (b->size.x / 2) &&
                      a->position.y - (a->size.y / 2) < b->position.y + (b->size.y / 2) &&
                      a->position.y + (a->size.y / 2) > b->position.y - (b->size.y / 2));
    return colliding;
}

Vector2 screen_to_world(Vector2 screen)
{
    Matrix4 proj = draw_frame.projection;
    Matrix4 view = draw_frame.camera_xform;
    float window_w = window.width;
    float window_h = window.height;

    // Normalize the mouse coordinates
    float ndc_x = (screen.x / (window_w * 0.5f)) - 1.0f;
    float ndc_y = (screen.y / (window_h * 0.5f)) - 1.0f;

    // Transform to world coordinates
    Vector4 world_pos = v4(ndc_x, ndc_y, 0, 1);
    world_pos = m4_transform(m4_inverse(proj), world_pos);
    world_pos = m4_transform(view, world_pos);

    return world_pos.xy;
}

Vector2 get_mouse_world_pos()
{
    return screen_to_world(v2(input_frame.mouse_x, input_frame.mouse_y));
}

int entry(int argc, char **argv)
{

    font = load_font_from_disk(STR("res/pong/PressStart2P-Regular.ttf"), get_heap_allocator());

    window.title = STR("Pong");
    window.point_width = 640;
    window.point_height = 360;
    window.point_x = 10;
    window.point_y = 100;
    window.clear_color = hex_to_rgba(0x000032ff);

    world = alloc(get_heap_allocator(), sizeof(World));
    world->ux_state = UX_main_menu;

    Entity *player_en = entity_create();
    setup_player(player_en);

    Entity *ball_en = entity_create();
    setup_ball(ball_en);

    Entity *enemy_en = entity_create();
    setup_enemy(enemy_en);

    float64 last_time = os_get_elapsed_seconds();
    bool bounced = false;
    float targetY = 0;

    while (!window.should_close)
    {
        reset_temporary_storage();
        float64 now = os_get_elapsed_seconds();
        if ((int)now != (int)last_time)
        {
            log("%.2f FPS", 1.0 / (now - last_time));
        }
        float64 delta_t = now - last_time;
        last_time = now;
        bounced = false;
        /***************************************************************
         * : Camera
         ****************************************************************/
        {
            // Scales the window size so the game strechs to fit any window size
            draw_frame.camera_xform = m4_make_scale(v3(1.0, 1.0, 1.0));
            draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_scale(v3(1.0 / (window.pixel_width / screen_width), 1.0 / (window.pixel_height / screen_height), 1.0)));
        }
        switch (world->ux_state)
        {
        case (UX_nil):
        {
            /***************************************************************
             * : Player
             ****************************************************************/
            player_en->velocity.y = is_key_down('W') - is_key_down('S');
            player_en->position = v2_add(player_en->position, v2_mulf(player_en->velocity, speed * delta_t));
            if (player_en->position.y + player_en->size.y / 2 >= (screen_height / 2))
            {
                player_en->position.y = (screen_height / 2) - player_en->size.y / 2;
            }
            if (player_en->position.y - player_en->size.y / 2 <= (-screen_height / 2))
            {
                player_en->position.y = -(screen_height / 2) + player_en->size.y / 2;
            }
            /***************************************************************
             * : Ball
             ****************************************************************/
            ball_en->velocity = v2_normalize(ball_en->velocity);
            ball_en->position = v2_add(ball_en->position, v2_mulf(ball_en->velocity, (1 * speed) * delta_t));
            if (is_colliding(player_en, ball_en))
            {
                float relative_collision_y = ball_en->position.y - player_en->position.y;
                float normalized_relative_collision = relative_collision_y / (player_en->size.y / 2);
                ball_en->velocity.x *= -1;
                ball_en->velocity.y += normalized_relative_collision * spin_factor;
                ball_en->position.x = player_en->position.x + (player_en->size.x / 2) + (ball_en->size.x / 2);
                bounced = true;
                play_one_audio_clip(STR("res/pong/pong.wav"));
            }
            if (is_colliding(enemy_en, ball_en))
            {
                float relative_collision_y = ball_en->position.y - enemy_en->position.y;
                float normalized_relative_collision = relative_collision_y / (enemy_en->size.y / 2);
                ball_en->velocity.x *= -1;
                ball_en->velocity.y += normalized_relative_collision * spin_factor;
                ball_en->position.x = enemy_en->position.x - enemy_en->size.x / 2 - (ball_en->size.x / 2);
                bounced = true;
                play_one_audio_clip(STR("res/pong/pong.wav"));
            }
            if (ball_en->position.y + (ball_en->size.y / 2) >= (screen_height / 2) || ball_en->position.y - (ball_en->size.y / 2) <= -(screen_height / 2))
            {
                ball_en->velocity.y *= -1;
                ball_en->position.y = screen_height / 2 * ball_en->position.y / fabsf(ball_en->position.y) + (ball_en->size.y / 2) * (ball_en->position.y > 0 ? -1 : 1);
                bounced = true;
                play_one_audio_clip(STR("res/pong/pong.wav"));
            }
            if (ball_en->position.x + (ball_en->size.x / 2) >= (screen_width / 2))
            {
                ball_en->position = v2(0, 0);
                player_en->score += 1;
                bounced = true;
                play_one_audio_clip(STR("res/pong/score.wav"));
            }
            if (ball_en->position.x - (ball_en->size.x / 2) <= -(screen_width / 2))
            {
                ball_en->position = v2(0, 0);
                enemy_en->score += 1;
                bounced = true;
                play_one_audio_clip(STR("res/pong/score.wav"));
            }
            /***************************************************************
             * : Enemy AI
             ****************************************************************/
            if (bounced)
            {
                float distanceY = ball_en->position.y - enemy_en->position.y;
                float timeToReachPaddle = (enemy_en->position.x - ball_en->position.x) / ball_en->velocity.x;
                float projectedBallY = ball_en->position.y + ball_en->velocity.y * timeToReachPaddle;
                float angleOffset = get_random_float32_in_range(-ai_range, ai_range) * (M_PI / 180.f);
                float adjustmentY = tan(angleOffset) * fabs(enemy_en->position.x - ball_en->position.x);

                targetY = projectedBallY + adjustmentY;
            }
            if (!(enemy_en->position.y + get_random_float32_in_range(0, enemy_en->size.y / 2) > targetY && enemy_en->position.y - get_random_float32_in_range(0, enemy_en->size.y / 2) < targetY))
            {
                if (enemy_en->position.y < targetY)
                {
                    enemy_en->position.y += speed * delta_t; // Move up
                }
                else if (enemy_en->position.y > targetY)
                {
                    enemy_en->position.y -= speed * delta_t; // Move down
                }
            }
            if (enemy_en->position.y + enemy_en->size.y / 2 >= (screen_height / 2))
            {
                enemy_en->position.y = (screen_height / 2) - enemy_en->size.y / 2;
            }
            if (enemy_en->position.y - enemy_en->size.y / 2 <= (-screen_height / 2))
            {
                enemy_en->position.y = -(screen_height / 2) + enemy_en->size.y / 2;
            }

            draw_line(v2(0, screen_height / 2), v2(0, -screen_height / 2), 1, hex_to_rgba(0xC8C8C8ff));
            draw_text(font, sprint(get_temporary_allocator(), STR("%d"), player_en->score), font_height, v2(-(screen_width / 4), -(screen_height / 2) + 25), v2(.5, .5), hex_to_rgba(0xf0f0f0ff));
            draw_text(font, sprint(get_temporary_allocator(), STR("%d"), enemy_en->score), font_height, v2((screen_width / 4), -(screen_height / 2) + 25), v2(.5, .5), hex_to_rgba(0xf0f0f0ff));

            for (int i = 0; i < MAX_ENTITY_COUNT; i++)
            {
                Entity *en = &world->entities[i];
                if (en->is_valid)
                {
                    switch (en->arch)
                    {
                    case arch_ball:
                    {
                        Matrix4 xform = m4_scalar(1.0);
                        float angle = atan2(en->velocity.y, en->velocity.x);
                        xform = m4_translate(xform, v3(en->position.x, en->position.y, 0));
                        xform = m4_translate(xform, v3(-en->size.x / 2, -en->size.y / 2, 0));
                        draw_rect_xform(xform, en->size, en->color);
                        break;
                    }
                    default:
                    {
                        Matrix4 xform = m4_scalar(1.0);
                        xform = m4_translate(xform, v3(en->position.x, en->position.y, 0));
                        xform = m4_translate(xform, v3(-en->size.x / 2, -en->size.y / 2, 0));
                        draw_rect_xform(xform, en->size, en->color);
                        break;
                    }
                    }
                }
            }
            if (player_en->score >= 5 || enemy_en->score >= 5)
            {
                world->ux_state = UX_game_over;
            }
            if (is_key_just_pressed(KEY_ESCAPE))
            {
                consume_key_just_pressed(KEY_ESCAPE);
                world->ux_state = UX_pause;
            }
            break;
        }
        case (UX_main_menu):
        {
            Vector2 button_pos = v2(-20, -50);

            draw_text(font, sprint(get_temporary_allocator(), STR("PONG")), font_height, v2(-screen_width / 6, 30), v2(.6, .6), COLOR_WHITE);

            Gfx_Text_Metrics text = draw_text_and_measure(font, sprint(get_temporary_allocator(), STR("START")), font_height, button_pos, v2(.2, .2), COLOR_WHITE);

            Vector2 topleft = v2(-10.0 + button_pos.x, 10 + text.visual_size.y + button_pos.y);
            Vector2 topright = v2(10 + text.visual_size.x + button_pos.x, 10 + text.visual_size.y + button_pos.y);
            Vector2 bottomleft = v2(-10.0 + button_pos.x, -10 + button_pos.y);
            Vector2 bottomright = v2(10 + text.visual_size.x + button_pos.x, -10 + button_pos.y);
            Vector2 mousePos = get_mouse_world_pos();

            if (mousePos.x >= bottomleft.x &&
                mousePos.x <= bottomleft.x + (bottomright.x - bottomleft.x) &&
                mousePos.y >= bottomleft.y &&
                mousePos.y <= bottomleft.y + (topleft.y - bottomleft.y))
            {
                draw_rect(bottomleft, v2_sub(topright, bottomleft), COLOR_WHITE);
                Gfx_Text_Metrics text = draw_text_and_measure(font, sprint(get_temporary_allocator(), STR("START")), font_height, button_pos, v2(.2, .2), COLOR_BLUE);
                if (is_key_just_pressed(MOUSE_BUTTON_LEFT))
                {
                    world->ux_state = UX_nil;
                }
            }
            draw_line(v2(bottomleft.x - 2.5, bottomleft.y), v2(bottomright.x + 2.5, bottomright.y), 5, COLOR_WHITE);
            draw_line(bottomleft, topleft, 5, COLOR_WHITE);
            draw_line(v2(topleft.x - 2.5, topleft.y), v2(topright.x + 2.5, topright.y), 5, COLOR_WHITE);
            draw_line(topright, bottomright, 5, COLOR_WHITE);

            break;
        }
        case (UX_pause):
        {
            draw_text(font, sprint(get_temporary_allocator(), STR("%d"), player_en->score), font_height, v2(-(screen_width / 4), -(screen_height / 2) + 25), v2(.6, .6), hex_to_rgba(0xf0f0f0ff));
            draw_text(font, sprint(get_temporary_allocator(), STR("%d"), enemy_en->score), font_height, v2((screen_width / 4) - 20, -(screen_height / 2) + 25), v2(.6, .6), hex_to_rgba(0xf0f0f0ff));
            draw_text(font, sprint(get_temporary_allocator(), STR("PAUSED")), font_height, v2(-screen_width / 4, 30), v2(.6, .6), COLOR_WHITE);
            draw_text(font, sprint(get_temporary_allocator(), STR("Press ESC To Resume")), font_height, v2(-screen_width / 4, -25), v2(.2, .2), COLOR_WHITE);

            for (int i = 0; i < MAX_ENTITY_COUNT; i++)
            {
                Entity *en = &world->entities[i];
                if (en->is_valid)
                {
                    switch (en->arch)
                    {
                    default:
                    {
                        Matrix4 xform = m4_scalar(1.0);
                        xform = m4_translate(xform, v3(en->position.x, en->position.y, 0));
                        xform = m4_translate(xform, v3(-en->size.x / 2, -en->size.y / 2, 0));
                        draw_rect_xform(xform, en->size, en->color);
                        break;
                    }
                    }
                }
            }
            if (is_key_just_pressed(KEY_ESCAPE))
            {
                consume_key_just_pressed(KEY_ESCAPE);
                world->ux_state = UX_nil;
            }
            break;
        }
        case (UX_game_over):
        {

            if (player_en->score >= 5)
            {
                draw_text(font, sprint(get_temporary_allocator(), STR("YOU WIN")), font_height, v2(-screen_width / 4, 30), v2(.6, .6), COLOR_WHITE);
            }
            else
            {
                draw_text(font, sprint(get_temporary_allocator(), STR("YOU LOSE")), font_height, v2(-screen_width / 4 - 30, 30), v2(.6, .6), COLOR_WHITE);
            }
            Vector2 button_pos = v2(-35, -50);
            Gfx_Text_Metrics text = draw_text_and_measure(font, sprint(get_temporary_allocator(), STR("RESTART")), font_height, button_pos, v2(.2, .2), COLOR_WHITE);

            Vector2 topleft = v2(-10.0 + button_pos.x, 10 + text.visual_size.y + button_pos.y);
            Vector2 topright = v2(10 + text.visual_size.x + button_pos.x, 10 + text.visual_size.y + button_pos.y);
            Vector2 bottomleft = v2(-10.0 + button_pos.x, -10 + button_pos.y);
            Vector2 bottomright = v2(10 + text.visual_size.x + button_pos.x, -10 + button_pos.y);
            Vector2 mousePos = get_mouse_world_pos();

            if (mousePos.x >= bottomleft.x &&
                mousePos.x <= bottomleft.x + (bottomright.x - bottomleft.x) &&
                mousePos.y >= bottomleft.y &&
                mousePos.y <= bottomleft.y + (topleft.y - bottomleft.y))
            {
                draw_rect(bottomleft, v2_sub(topright, bottomleft), COLOR_WHITE);
                Gfx_Text_Metrics text = draw_text_and_measure(font, sprint(get_temporary_allocator(), STR("RESTART")), font_height, button_pos, v2(.2, .2), COLOR_BLUE);

                if (is_key_just_pressed(MOUSE_BUTTON_LEFT))
                {
                    setup_player(player_en);
                    setup_enemy(enemy_en);
                    setup_ball(ball_en);
                    world->ux_state = UX_nil;
                }
            }
            draw_line(v2(bottomleft.x - 2.5, bottomleft.y), v2(bottomright.x + 2.5, bottomright.y), 5, COLOR_WHITE);
            draw_line(bottomleft, topleft, 5, COLOR_WHITE);
            draw_line(v2(topleft.x - 2.5, topleft.y), v2(topright.x + 2.5, topright.y), 5, COLOR_WHITE);
            draw_line(topright, bottomright, 5, COLOR_WHITE);

            break;
        }
        default:
        {
            break;
        }
        }
        os_update();
        gfx_update();
    }
    return 0;
}