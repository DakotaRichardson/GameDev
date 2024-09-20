const float screen_width = 320.0f;
const float screen_height = 180.0f;
const float paddle_padding = 20.0f;
const float speed = 100.0f;

Gfx_Font *
    font;
u32 font_height = 48;

typedef enum EntityArchetype
{
    arch_nil = 0,
    arch_player = 1,
    arch_ball = 2,
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
} Entity;

#define MAX_ENTITY_COUNT 1024

typedef enum UXState
{
    UX_nil,
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
}

void setup_ball(Entity *en)
{
    en->arch = arch_ball;
    en->position = v2(0, 0);
    en->velocity = v2(-1, 1);
    en->size = v2(5, 5);
    en->color = COLOR_WHITE;
}

bool is_colliding(Entity *a, Entity *b)
{
    bool colliding = (a->position.x - (a->size.x / 2) < b->position.x + (b->size.x / 2) &&
                      a->position.x + (a->size.x / 2) > b->position.x - (b->size.x / 2) &&
                      a->position.y - (a->size.y / 2) < b->position.y + (b->size.y / 2) &&
                      a->position.y + (a->size.y / 2) > b->position.y - (b->size.y / 2));
    return colliding;
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

    Entity *player_en = entity_create();
    setup_player(player_en);

    Entity *ball_en = entity_create();
    setup_ball(ball_en);

    float64 last_time = os_get_elapsed_seconds();

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

        /***************************************************************
         * Camera
         ****************************************************************/
        {
            // Scales the window size so the game strechs to fit any window size
            draw_frame.camera_xform = m4_make_scale(v3(1.0, 1.0, 1.0));
            draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_scale(v3(1.0 / (window.pixel_width / screen_width), 1.0 / (window.pixel_height / screen_height), 1.0)));
        }
        /***************************************************************
         * Player
         ****************************************************************/
        {
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
        }
        /***************************************************************
         * Ball
         ****************************************************************/
        {
            ball_en->velocity = v2_normalize(ball_en->velocity);
            ball_en->position = v2_add(ball_en->position, v2_mulf(ball_en->velocity, (1 * speed) * delta_t));
            if (ball_en->position.y + (ball_en->size.y / 2) >= (screen_height / 2) || ball_en->position.y - (ball_en->size.y / 2) <= -(screen_height / 2))
            {
                ball_en->velocity.y *= -1;
                ball_en->position.y = screen_height / 2 * ball_en->position.y / fabsf(ball_en->position.y) + (ball_en->size.y / 2) * (ball_en->position.y > 0 ? -1 : 1);
            }
            if (ball_en->position.x + (ball_en->size.x / 2) >= (screen_width / 2))
            {
                ball_en->position = v2(0, 0);
            }
            if (ball_en->position.x - (ball_en->size.x / 2) <= -(screen_width / 2))
            {
                ball_en->position = v2(0, 0);
            }
        }
        /***************************************************************
         * Render Entities
         ****************************************************************/
        {
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
        }

        os_update();
        gfx_update();
    }
    return 0;
}