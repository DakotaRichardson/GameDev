const float screen_width = 320.0f;
const float screen_height = 176.0f;
const int tile_size = 8;

Gfx_Font *
    font;
u32 font_height = 48;

Vector2 screen_to_world()
{
    float mouse_x = input_frame.mouse_x;
    float mouse_y = input_frame.mouse_y;
    Matrix4 proj = draw_frame.projection;
    Matrix4 view = draw_frame.view;
    float window_w = window.width;
    float window_h = window.height;

    float ndc_x = (mouse_x / (window_w * 0.5f)) - 1.0f;
    float ndc_y = (mouse_y / (window_h * 0.5f)) - 1.0f;

    Vector4 world_pos = v4(ndc_x, ndc_y, 0, 1);
    world_pos = m4_transform(m4_inverse(proj), world_pos);
    world_pos = m4_transform(view, world_pos);

    return (Vector2){world_pos.x, world_pos.y};
}

int entry(int argc, char **argv)
{
    font = load_font_from_disk(STR("res/pong/PressStart2P-Regular.ttf"), get_heap_allocator());

    window.title = STR("DDA RayCast Demo");
    window.point_width = 640;
    window.point_height = 352;
    window.point_x = 10;
    window.point_y = 100;
    window.clear_color = hex_to_rgba(0x000000ff);

    Vector2 player = v2(0, 0);
    Vector2 MapSize = v2(40, 22);
    int map[(int)MapSize.y][(int)MapSize.x];
    memset(map, 0, sizeof(map));

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
            draw_frame.camera_xform = m4_make_scale(v3(1.0, 1.0, 1.0));
            draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_scale(v3(1.0 / (window.pixel_width / screen_width), 1.0 / (window.pixel_height / screen_height), 1.0)));
        }
        /***************************************************************
         * Player Movement
         ****************************************************************/
        {
            if (is_key_down('W'))
                player.y += 25.0f * delta_t;
            if (is_key_down('S'))
                player.y -= 25.0f * delta_t;
            if (is_key_down('D'))
                player.x += 25.0f * delta_t;
            if (is_key_down('A'))
                player.x -= 25.0f * delta_t;
        }
        /***************************************************************
         * Ray Cast
         ****************************************************************/
        Vector2 rayStart = v2((player.x + (screen_width / 2)) / tile_size, (player.y + (screen_height / 2)) / tile_size);
        Vector2 mousePos = screen_to_world();
        Vector2 mouseTile = v2((mousePos.x + (screen_width / 2)) / tile_size, (mousePos.y + (screen_height / 2)) / tile_size);
        Vector2 rayDirection = v2_normalize(v2_sub(mouseTile, rayStart));

        if (is_key_down(MOUSE_BUTTON_RIGHT))
        {
            map[(int)round(mouseTile.y)][(int)round(mouseTile.x)] = 1;
        }

        Vector2 RayUnitStepSize = {INFINITY, INFINITY};
        if (rayDirection.x != 0)
            RayUnitStepSize.x = sqrt(1 + (rayDirection.y / rayDirection.x) * (rayDirection.y / rayDirection.x));
        if (rayDirection.y != 0)
            RayUnitStepSize.y = sqrt(1 + (rayDirection.x / rayDirection.y) * (rayDirection.x / rayDirection.y));

        Vector2 MapCheck = rayStart;
        Vector2 RayLength1D;
        Vector2 Step;

        if (rayDirection.x < 0)
        {
            Step.x = -1;
            RayLength1D.x = (rayStart.x - (int)rayStart.x) * RayUnitStepSize.x;
        }
        else
        {
            Step.x = 1;
            RayLength1D.x = ((1.0f - (rayStart.x - (int)rayStart.x))) * RayUnitStepSize.x;
        }

        if (rayDirection.y < 0)
        {
            Step.y = -1;
            RayLength1D.y = (rayStart.y - (int)rayStart.y) * RayUnitStepSize.y;
        }
        else
        {
            Step.y = 1;
            RayLength1D.y = ((1.0f - (rayStart.y - (int)rayStart.y))) * RayUnitStepSize.y;
        }

        bool bTileFound = false;
        float fMaxDistance = 100.0f;
        float fDistance = 0.0f;

        while (!bTileFound && fDistance < fMaxDistance)
        {
            if (RayLength1D.x < RayLength1D.y)
            {
                MapCheck.x += Step.x;
                fDistance = RayLength1D.x;
                RayLength1D.x += RayUnitStepSize.x;
            }
            else
            {
                MapCheck.y += Step.y;
                fDistance = RayLength1D.y;
                RayLength1D.y += RayUnitStepSize.y;
            }

            if (MapCheck.x >= 0 && MapCheck.x < MapSize.x && MapCheck.y >= 0 && MapCheck.y < MapSize.y)
            {
                if (map[(int)MapCheck.y][(int)MapCheck.x] == 1)
                {
                    bTileFound = true;
                }
            }
        }

        Vector2 Intersection;
        if (bTileFound)
        {
            Intersection = v2_add(rayStart, v2_mulf(rayDirection, fDistance));
        }

        /***************************************************************
         * Render Entities
         ****************************************************************/
        {
            for (int y = 0; y < MapSize.y; y++)
            {
                for (int x = 0; x < MapSize.x; x++)
                {
                    int cell = map[y][x];
                    float screen_x = (x * tile_size) - (screen_width / 2);
                    float screen_y = (y * tile_size) - (screen_height / 2);

                    Matrix4 xform = m4_scalar(1.0);
                    xform = m4_translate(xform, v3(screen_x, screen_y, 0));
                    draw_rect_xform(xform, v2(tile_size, tile_size), hex_to_rgba(0x585858ff));
                    xform = m4_scalar(1.0);
                    xform = m4_translate(xform, v3(screen_x, screen_y, 0));
                    draw_rect_xform(xform, v2(tile_size - 1, tile_size - 1), COLOR_BLACK);

                    if (cell == 1)
                    {
                        xform = m4_scalar(1.0);
                        xform = m4_translate(xform, v3(screen_x, screen_y, 0));
                        draw_rect_xform(xform, v2(tile_size - 1, tile_size - 1), COLOR_BLUE);
                    }
                }
            }

            if (is_key_down(MOUSE_BUTTON_LEFT))
            {
                draw_line(player, mousePos, 1, COLOR_WHITE);

                if (bTileFound)
                {
                    float screen_x = (Intersection.x * tile_size) - (screen_width / 2);
                    float screen_y = (Intersection.y * tile_size) - (screen_height / 2);
                    Matrix4 xform = m4_scalar(1.0);
                    xform = m4_translate(xform, v3(screen_x, screen_y, 0));
                    xform = m4_translate(xform, v3(-2.0, -2.0, 0));
                    draw_circle_xform(xform, v2(4.0, 4.0), hex_to_rgba(0xe8d282ff));
                }
            }

            Matrix4 xform = m4_scalar(1.0);
            xform = m4_translate(xform, v3(player.x, player.y, 0));
            xform = m4_translate(xform, v3(-4.0, -4.0, 0));
            draw_circle_xform(xform, v2(8.0, 8.0), COLOR_RED);
            xform = m4_scalar(1.0);
            xform = m4_translate(xform, v3(mousePos.x, mousePos.y, 0));
            xform = m4_translate(xform, v3(-4.0, -4.0, 0));
            draw_circle_xform(xform, v2(8.0, 8.0), COLOR_GREEN);

            draw_line(player, v2_add(player, v2_mulf(rayDirection, 10)), 1, hex_to_rgba(0xfa6e79ff));
        }

        os_update();
        gfx_update();
    }
    return 0;
}
