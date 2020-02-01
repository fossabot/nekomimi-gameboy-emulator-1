#include "emulator-form.h"

using namespace gameboy;

bool Emulatorform::get_joypad_input(Joypad &joypad, Memory &mem)
{

    // W-Up
    // S-Down
    // A-Left
    // D-Right
    // J-A
    // K-B
    // T-Select
    // Enter-Start
    // Q-Quick Save
    // Y-Quick Load
    // P-Quit and Save
    // L-Fast Foward

    while (SDL_PollEvent(&(Emulatorform::joypad_event)))
    {
        joypad.temp_ff00 = mem.get_memory_byte(0xFF00);
        joypad.temp_ff00 &= 0x30;
        mem.set_memory_byte(0xFF00, joypad.temp_ff00);

        if (joypad_event.type == SDL_QUIT)
            return false;

        // when you hit keys on keyboard
        if (joypad_event.type == SDL_KEYDOWN)
        {
            switch (joypad_event.key.keysym.sym)
            {
            // Quit Emulation Percess
            case SDLK_ESCAPE:
                printf("Now quitting.\n");
                return false;
                break;

            // for column 1 (Directions)
            // hit RIGHT
            case SDLK_d:
                joypad.key_column = 0x20;
                joypad.column_direction = 1;
                joypad.keys_directions &= 0xE;
                break;

            // hit LEFT
            case SDLK_a:
                joypad.key_column = 0x20;
                joypad.column_direction = 1;
                joypad.keys_directions &= 0xD;
                break;

            // hit UP
            case SDLK_w:
                joypad.key_column = 0x20;
                joypad.column_direction = 1;
                joypad.keys_directions &= 0xB;
                break;

            // hit DOWN
            case SDLK_s:
                joypad.key_column = 0x20;
                joypad.column_direction = 1;
                joypad.keys_directions &= 0x7;
                break;

            // for column 0 (Controls)
            // hit A
            case SDLK_j:
                joypad.key_column = 0x10;
                joypad.column_controls = 1;
                joypad.keys_controls &= 0xE;
                break;

            // hit B
            case SDLK_k:
                joypad.key_column = 0x10;
                joypad.column_controls = 1;
                joypad.keys_controls &= 0xD;
                break;

            // hit SELECT
            case SDLK_t:
                joypad.key_column = 0x10;
                joypad.column_controls = 1;
                joypad.keys_controls &= 0xB;
                break;

            // hit START
            case SDLK_RETURN:
                joypad.key_column = 0x10;
                joypad.column_controls = 1;
                joypad.keys_controls &= 0x7;
                break;

            // hit Quick Save
            case SDLK_q:
                joypad.save_flag = 1;
                printf("Will save before next poll...\n");
                break;

            // hit Quick Load
            case SDLK_y:
                joypad.load_flag = 1;
                printf("Will load before next poll...\n");
                break;

            // Quit and Save
            case SDLK_p:
                printf("Quit and save.\n");
                return false;
                break;

            // hit Fast Foward
            case SDLK_l:
                joypad.fast_forward_flag = 1;
                printf("Triggering fast foward...\n");
                break;
            }
            //joypad.joypad_interrupts(mem);
        }
        // when you release keys on keyboard
        else if (joypad_event.type == SDL_KEYUP)
        {
            switch (joypad_event.key.keysym.sym)
            {
            // for column 1 (Directions)
            // release RIGHT
            case SDLK_d:
                joypad.key_column = 0x20;
                joypad.column_direction = 1;
                joypad.keys_directions |= 0x1;
                break;

            // release LEFT
            case SDLK_a:
                joypad.key_column = 0x20;
                joypad.column_direction = 1;
                joypad.keys_directions |= 0x2;
                break;

            // release UP
            case SDLK_w:
                joypad.key_column = 0x20;
                joypad.column_direction = 1;
                joypad.keys_directions |= 0x4;
                break;

            // release DOWN
            case SDLK_s:
                joypad.key_column = 0x20;
                joypad.column_direction = 1;
                joypad.keys_directions |= 0x8;
                break;

            // for column 0 (Controls)
            // release A
            case SDLK_j:
                joypad.key_column = 0x10;
                joypad.column_controls = 1;
                joypad.keys_controls |= 0x1;
                break;

            // release B
            case SDLK_k:
                joypad.key_column = 0x10;
                joypad.column_controls = 1;
                joypad.keys_controls |= 0x2;
                break;

            // release SELECT
            case SDLK_t:
                joypad.key_column = 0x10;
                joypad.column_controls = 1;
                joypad.keys_controls |= 0x4;
                break;

            // release START
            case SDLK_RETURN:
                joypad.key_column = 0x10;
                joypad.column_controls = 1;
                joypad.keys_controls |= 0x8;
                break;
            }
        }
        else if( joypad_event.type == SDL_JOYAXISMOTION )
        {
            //Motion on controller 0
            if( joypad_event.jaxis.which == 0 )
            {
                //X axis motion
                if( joypad_event.jaxis.axis == 0 )
                {
                    //Left of dead zone,
                    if( joypad_event.jaxis.value < -JOYSTICK_DEAD_ZONE )
                    {
                        joypad.key_column = 0x20;
                        joypad.column_direction = 1;
                        joypad.keys_directions &= 0xD;
                        break;
                    }
                    //Right of dead zone
                    else if( joypad_event.jaxis.value > JOYSTICK_DEAD_ZONE )
                    {
                        joypad.key_column = 0x20;
                        joypad.column_direction = 1;
                        joypad.keys_directions &= 0xE;
                        break;
                    }
                    else
                    {
                        joypad.key_column = 0x20;
                        joypad.column_direction = 1;
                        joypad.keys_directions |= 0x2;
                        joypad.keys_directions |= 0x1;
                    }
                }
                //Y axis motion
                else if( joypad_event.jaxis.axis == 1 )
                {
                    //Below of dead zone
                    if( joypad_event.jaxis.value < -JOYSTICK_DEAD_ZONE )
                    {
                        joypad.key_column = 0x20;
                        joypad.column_direction = 1;
                        joypad.keys_directions &= 0xB;
                        break;
                    }
                    //Above of dead zone
                    else if( joypad_event.jaxis.value > JOYSTICK_DEAD_ZONE )
                    {
                        joypad.key_column = 0x20;
                        joypad.column_direction = 1;
                        joypad.keys_directions &= 0x7;
                        break;
                    }
                    else
                    {
                        joypad.key_column = 0x20;
                        joypad.column_direction = 1;
                        joypad.keys_directions |= 0x8;
                        joypad.keys_directions |= 0x4;
                    }
                }
            }
        }
        else if( joypad_event.type == SDL_JOYBUTTONDOWN)
        {
            switch (joypad_event.jbutton.button)
            {
            case SDL_CONTROLLER_BUTTON_A:
                joypad.key_column = 0x10;
                joypad.column_controls = 1;
                joypad.keys_controls &= 0xE;
                break;

            case SDL_CONTROLLER_BUTTON_B:
                joypad.key_column = 0x10;
                joypad.column_controls = 1;
                joypad.keys_controls &= 0xD;
                break;

            case SDL_CONTROLLER_BUTTON_START:
                joypad.key_column = 0x10;
                joypad.column_controls = 1;
                joypad.keys_controls &= 0x7;
                break;

            // hit Quick Save
            case SDL_CONTROLLER_BUTTON_X:
                joypad.save_flag = 1;
                printf("Will save before next poll...\n");
                break;

            // hit Quick Load
            case SDL_CONTROLLER_BUTTON_Y:
                joypad.load_flag = 1;
                printf("Will load before next poll...\n");
                break;

            default:
                break;
            }
        }
        else if( joypad_event.type == SDL_JOYBUTTONUP)
        {
            switch (joypad_event.jbutton.button)
            {
            case SDL_CONTROLLER_BUTTON_A:
                joypad.key_column = 0x10;
                joypad.column_controls = 1;
                joypad.keys_controls |= 0x1;
                break;

            case SDL_CONTROLLER_BUTTON_B:
                joypad.key_column = 0x10;
                joypad.column_controls = 1;
                joypad.keys_controls |= 0x2;
                break;

            case SDL_CONTROLLER_BUTTON_START:
                joypad.key_column = 0x10;
                joypad.column_controls = 1;
                joypad.keys_controls |= 0x8;
                break;
            default:
                break;
            }
        }
    }
    joypad.write_result(mem);
    joypad.reset_joypad();
    return true;
}

bool Emulatorform::refresh_surface(void)
{
    SDL_UpdateWindowSurface(Emulatorform::emulator_window);
    return false;
}

void Emulatorform::set_pixel_color(uint8_t pos_x, uint8_t pos_y, uint8_t color, uint8_t scale)
{
    SDL_UnlockSurface(Emulatorform::emulator_window_surface);
    auto format = Emulatorform::emulator_window_surface->format;
    uint32_t *pixels = (Uint32 *)Emulatorform::emulator_window_surface->pixels;
    for (int scale_x = 0; scale_x < scale; scale_x++)
    {
        for (int scale_y = 0; scale_y < scale; scale_y++)
        {
            pixels[(pos_y * scale + scale_y) * (SCREEN_WIDTH * scale) + pos_x * scale + scale_x] = SDL_MapRGB(format, Emulatorform::color_palatte[color][0], Emulatorform::color_palatte[color][1], Emulatorform::color_palatte[color][2]);
        }
    }
}

void Emulatorform::create_window(uint16_t on_screen_window_width, uint16_t on_screen_window_height, std::string on_screen_title, uint8_t rgb_red, uint8_t rgb_green, uint8_t rgb_blue, uint8_t scale)
{
    // init video and joystick
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);

    // get physical devices' resolution
    uint16_t physical_device_res_width = 0;
    uint16_t physical_device_res_height = 0;
    SDL_GetDesktopDisplayMode(0, &(Emulatorform::physical_device_display_mode));
    physical_device_res_width = physical_device_display_mode.w;
    physical_device_res_height = physical_device_display_mode.h;

    printf("Physical device resolution is %d*%d\n", physical_device_res_width, physical_device_res_height);

    // now create window
    Emulatorform::emulator_window = SDL_CreateWindow(on_screen_title.c_str(), physical_device_res_width / 2 - SCREEN_WIDTH * scale / 2, physical_device_res_height / 2 - SCREEN_HEIGHT * scale / 2, on_screen_window_width * scale, on_screen_window_height * scale, SDL_WINDOW_SHOWN);

    // get the surface
    Emulatorform::emulator_window_surface = SDL_GetWindowSurface(Emulatorform::emulator_window);

    // fill window with colors
    SDL_FillRect(Emulatorform::emulator_window_surface, NULL, SDL_MapRGB(Emulatorform::emulator_window_surface->format, rgb_red, rgb_green, rgb_blue));

    //  update windows surface to show the color
    SDL_UpdateWindowSurface(Emulatorform::emulator_window);

    if( SDL_NumJoysticks() < 1 )
    {
        printf( "[No joysticks connected!]\n" );
    }
    else
    {
        //Load joystick
        game_controller = SDL_JoystickOpen( 0 );
        if( game_controller == NULL )
        {
            printf( "Warning: Unable to open game controller! SDL Error: %s\n", SDL_GetError() );
        }
        else
        {
            printf("Joystick connected!\n");
        }
    }
    printf("\n\nCurrent Key Mapping:\n");
    printf("W - UP       S - DOWN    A - LEFT   D - RIGHT\n");
    printf("J - A        K - B\n");
    printf("T - Select   Enter - Start\n");
    printf("Q - Quick Save\n");
    printf("Y - Quick Load\n");
    printf("P - Quit and Save\n\n");
    printf("\n\nCurrent Joystick Mapping:\n");
    printf("Left Analog Stick: Directions\n");
    printf("START - Start         Tips: In DS3 Controller, use SELECT for START\n");
    printf("A - A        B - B\n");
    printf("X - Quick Save\n");
    printf("Y - Quick Load\n");
    printf("P - Quit and Save\n\n");
    printf("Note: We recommend you to load when the game actually start, or you may stuck.\n");

}

void Emulatorform::destroy_window(void)
{
    SDL_FreeSurface(Emulatorform::emulator_window_surface);
    SDL_JoystickClose( game_controller );
    game_controller = NULL;
    SDL_Quit();
}
