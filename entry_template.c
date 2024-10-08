const float screen_width = 320.0f;
const float screen_height = 180.0f;

Gfx_Font *
    font;
u32 font_height = 48;

typedef enum EntityArchetype
{
    arch_nil = 0,
    arch_player = 1,
    ARCH_MAX,

} EntityArchetype;

typedef struct Entity
{
    bool is_valid;
    EntityArchetype arch;
    Vector2 position;
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
    en->position = v2(0, 0);
}

int entry(int argc, char **argv)
{

    font = load_font_from_disk(STR("res/pong/PressStart2P-Regular.ttf"), get_heap_allocator());

    window.title = STR("Template");
    window.point_width = 640;
    window.point_height = 360;
    window.point_x = 10;
    window.point_y = 100;
    window.clear_color = hex_to_rgba(0x000000ff);

    world = alloc(get_heap_allocator(), sizeof(World));

    Entity *player_en = entity_create();
    setup_player(player_en);

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
                        draw_rect_xform(xform, v2(10.0, 10.0), COLOR_WHITE);
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