#include "ppu.h"

using gameboy::Emulatorform;
using gameboy::Memory;
using gameboy::Ppu;
using gameboy::PpuMode;

void Ppu::ppu_main(uint8_t clocks, uint8_t speed_hack, Memory &mem, Emulatorform &form, uint8_t scale)
{
    reset_interrupt_registers(mem);
    // sync with system cycles
    add_time(clocks * speed_hack);

    // 1 clock == 4 dots
    // 0~20*4-1 (0~79) OAM Search
    // 20*4~(20+43)*4-1 (80~251) Pixel Transfer
    // (20+43)*4~(20+43+51)*4-1) (252~455) H-Blank
    // after SCREEN_HEIGHT (144) lines, we update the buffer and flush it to the screen

    if (current_mode == PpuMode::mode_oam_search)
    {
        if (ppu_inner_clock >= 79)
        {
            ppu_inner_clock = ppu_inner_clock - 79;
            set_mode(PpuMode::mode_pixel_transfer, mem);
            //oam_search(mem);
        }
    }

    else if (current_mode == PpuMode::mode_pixel_transfer)
    {
        if (ppu_inner_clock >= 172)
        {
            ppu_inner_clock = ppu_inner_clock - 172;
            set_mode(PpuMode::mode_hblank, mem);
            pixel_transfer(mem);
        }
    }

    else if (current_mode == PpuMode::mode_hblank)
    {
        if (ppu_inner_clock >= 205)
        {
            ppu_inner_clock = ppu_inner_clock - 205;

            h_blank(speed_hack, mem, form, scale);

            uint8_t ly_byte = mem.get_memory_byte(LY_ADDRESS);
            // if LY >= SCREEN_HEIGHT enter vblank in next loop, flush buffer to screen now
            if (ly_byte >= SCREEN_HEIGHT)
            {
                set_mode(PpuMode::mode_vblank, mem);
                //form.refresh_surface();
                ready_to_refresh = true;
            }

            //if not, go on to next line
            else
            {
                set_mode(PpuMode::mode_oam_search, mem);
            }
        }
    }
    else if (current_mode == PpuMode::mode_vblank)
    {
        // enter 10 lines of v_blank
        if (ppu_inner_clock < 4560)
        {
            v_blank(mem);
        }

        // when reach the end, move to OAM Search in the next loop
        if (ppu_inner_clock >= 4560)
        {
            set_mode(PpuMode::mode_oam_search, mem);
            ppu_inner_clock = ppu_inner_clock - 4560;

            // reset ly
            mem.set_memory_byte(LY_ADDRESS, 0);
        }
    }

    // update LYC at the end of each loop
    update_lyc(mem);
}

void Ppu::oam_search(Memory &mem)
{
    // from OAM_TABLE_INITIAL_ADDDRESS to 0xFE9F
    // 40 sprites
    // each sprite occupy 4 bytes


}

void Ppu::pixel_transfer(Memory &mem)
{
    uint8_t dma_byte = mem.get_memory_byte(DMA_ADDRESS);
    for (uint8_t bytes = 0; bytes <= 0xA0; bytes++)
    {
        uint16_t dma_address_source = dma_byte * 0x0100 + bytes;
        uint16_t dma_address_destin = OAM_TABLE_INITIAL_ADDDRESS + bytes;
        mem.set_memory_byte(dma_address_destin, mem.get_memory_byte(dma_address_source));
    }
}

void Ppu::h_blank(uint8_t speed_hack, Memory &mem, Emulatorform &form, uint8_t scale)
{
    // get current line
    uint8_t ly_byte = mem.get_memory_byte(LY_ADDRESS);

    uint8_t lcdc_byte = mem.get_memory_byte(LCDC_ADDRESS);

    // draw current line
    if (lcdc_byte & 0x01)
        draw_line(ly_byte, mem, form, scale);

    // write LY value into memory
    ly_byte++;
    mem.set_memory_byte(LY_ADDRESS, ly_byte);
}

void Ppu::v_blank(Memory &mem)
{
    uint8_t ly_byte = (ppu_inner_clock / 456 + SCREEN_HEIGHT);
    mem.set_memory_byte(LY_ADDRESS, ly_byte);
}

void Ppu::set_mode(PpuMode mode, Memory &mem)
{
    if (current_mode == mode)
    {
        return;
    }
    current_mode = mode;

    uint8_t stat_byte = mem.get_memory_byte(STAT_ADDRESS);
    uint8_t interrupt_flag_byte = mem.get_memory_byte(IF_ADDRESS);

    // change and write mode in STAT to registers
    // set STAT
    // first, unset bit 0 and bit 1
    stat_byte &= 0xFC;
    // second, set bit 0 and bit 1 according to mode
    stat_byte |= (mode & 0x03);

    // v_blank Interrupt
    if (mode == PpuMode::mode_vblank)
    {
        interrupt_flag_byte |= 0x01;
    }

    // LCDC Status interrupt
    // if current mode equal with the mode represented by specific bit in STAT
    // bit 5: OAM search
    // bit 4: V-Blank
    // bit 3: H-Blank
    if (
        ((mode == PpuMode::mode_hblank) && (stat_byte & 0x0F)) ||
        ((mode == PpuMode::mode_vblank) && (stat_byte & 0x10)) ||
        ((mode == PpuMode::mode_oam_search) && (stat_byte & 0x20))
    )

    {
        interrupt_flag_byte |= 0x02;
    }

    // write back to memory
    mem.set_memory_byte(IF_ADDRESS, interrupt_flag_byte);
    mem.set_memory_byte(STAT_ADDRESS, stat_byte);
}

void Ppu::draw_line(uint8_t line_number_y, Memory &mem, Emulatorform &form, uint8_t scale)
{
    if (line_number_y >= SCREEN_HEIGHT)
    {
        return;
    }

    uint8_t lcdc_byte = mem.get_memory_byte(LCDC_ADDRESS);

    // judge bit 0 in LCDC (BG & Window Enable)
    bool render_background = lcdc_byte & 0x01;

    // if bit 0 in LCDC (BG Enable) is true
    // put all background data into buffer

    // for background map
    // examples:
    // now we are rendering line 1
    // we *read* 0x9800, *find* a tile #
    // for example the # is 0x04
    // we *find* the 0x04 tile with line 1 in address 0x8040 and 0x8041
    // 0x8040 = 0x8000 + 0x04 * 0x10

    // judge bit 4 in LCDC (BG & Windows Tile Data)
    bool background_window_tile_data_address_flag = lcdc_byte & 0x10;
    // if the flag is false, use signed index
    uint16_t background_window_tile_data_start_address = (background_window_tile_data_address_flag) ? 0x8000 : 0x9000;

    // render background
    if (render_background)
    {
        // get background tile data address
        // 0: $8800-$97FF,  tile # ranging 0~255, #0 is 0x8800
        // 1: $8000-$8FFF,  tile # ranging -128~127, #0 is 0x9000
        // but from the FIFO example in the Ultimate GameBoy talk
        // we can simply use #0 for 8000
        // example is in how to put one line into buffer...

        // 4096 bytes in all, 16 bytes per tile, one 8*8 image
        // which means, 1 line (8 pixels) occupy 2 bytes
        // if you want to read the i-th line in the tile #j, you should read address:
        // TILE_START_ADDRESS + j * 16 +(i-1) * 2 and TILE_START_ADDRESS + j*16 + (i-1) * 2 + 1




        // get background tile map Address
        // 0: $9800-$9BFF
        // 1: $9C00-$9FFF
        // judge bit 3 in LCDC (BG Tile Map Address)
        uint16_t background_tile_map_start_address = (lcdc_byte & 0x08) ? 0x9C00 : 0x9800;

        uint8_t SCY = mem.get_memory_byte(SCY_ADDRESS);
        uint8_t SCX = mem.get_memory_byte(SCX_ADDRESS);
        int y = (line_number_y + SCY) % 256; //locate background in background map

        int last_tile_x = -1;
        //it is strange, this two should be put outside.
        uint8_t tile_data_bytes_line_one = 0x00;
        uint8_t tile_data_bytes_line_two = 0x00;

        for (int i = 0; i < SCREEN_WIDTH; i++)
        {

            int x = (i + SCX) % 256;
            int tile_x = x / 8; //8 pixels per tile
            int tile_y = y / 8;
            int pixel_x = 8 - x % 8 - 1;
            int pixel_y = y % 8;

            // render a new tile unless we get to the last
            if (tile_x != last_tile_x)
            {
                // get original tile number
                int tmp_tile_index = mem.get_memory_byte(background_tile_map_start_address + (tile_y * 32) + tile_x);

                // judge whether the number is unsigned
                int tile_index = tmp_tile_index;

                if (!background_window_tile_data_address_flag)
                {
                    // switch signed tile index
                    tile_index = (int8_t)tmp_tile_index;
                }

                // get tile data
                tile_data_bytes_line_one = mem.get_memory_byte(background_window_tile_data_start_address + (tile_index * 16) + (pixel_y * 2));
                tile_data_bytes_line_two = mem.get_memory_byte(background_window_tile_data_start_address + (tile_index * 16) + (pixel_y * 2) + 1);
                last_tile_x = tile_x;
            }

            // mix pixel color of two lines
            int color = mix_tile_colors(pixel_x, tile_data_bytes_line_one, tile_data_bytes_line_two);
            // directly set color
            form.set_pixel_color(i, line_number_y, color, scale);
        }
    }


    //render window
    //judge bit 5 and bit 0
    bool render_window = lcdc_byte & 0x20;

    if (render_background && render_window)
    {
        // judge bit 6 in LCDC (Window Tile Map Address)
        uint16_t window_tile_map_start_address = (lcdc_byte & 0x40) ? 0x9C00 : 0x9800;

        uint8_t WY = mem.get_memory_byte(WY_ADDRESS);
        int y = line_number_y - WY;

        //whether we should draw window at this line
        if ((line_number_y - WY) > 0)
        {
            uint8_t WX = mem.get_memory_byte(WX_ADDRESS);
            if (WX < 7)
            {
                WX = 7;
            }
            //WX is offset from absolute screen coordinates by 7.
            uint8_t absolute_WX = WX - 7;

            uint8_t tile_data_bytes_line_one = 0x00;
            uint8_t tile_data_bytes_line_two = 0x00;

            int last_tile_x = -1;

            for (int i = 0; i < SCREEN_WIDTH; i++)
            {
                if (i < absolute_WX)
                {
                    continue;
                }
                int x = i - absolute_WX;
                int tile_x = x / 8; //8 pixels per tile
                int tile_y = y / 8;
                int pixel_x = 8 - x % 8 - 1;
                int pixel_y = y % 8;

                // render a new tile unless we get to the last
                if (tile_x != last_tile_x)
                {
                    // get original tile number
                    int tmp_tile_index = mem.get_memory_byte(window_tile_map_start_address + (tile_y * 32) + tile_x);

                    // judge whether the number is unsigned
                    int tile_index = tmp_tile_index;

                    if (!background_window_tile_data_address_flag)
                    {
                        // switch signed tile index
                        tile_index = (int8_t)tmp_tile_index;
                    }

                    // get tile data
                    tile_data_bytes_line_one = mem.get_memory_byte(background_window_tile_data_start_address + (tile_index * 16) + (pixel_y * 2));
                    tile_data_bytes_line_two = mem.get_memory_byte(background_window_tile_data_start_address + (tile_index * 16) + (pixel_y * 2) + 1);
                    last_tile_x = tile_x;
                }

                // mix pixel color of two lines
                int color = mix_tile_colors(pixel_x, tile_data_bytes_line_one, tile_data_bytes_line_two);

                // directly set color
                form.set_pixel_color(i, line_number_y, color, scale);
            }
        }
    }

    // Sprites
    bool OBJ_enable = lcdc_byte & 0x02;

    if (OBJ_enable)
    {
        //sprite_height from LCDC bit 2
        uint8_t sprite_height = 8 * (((lcdc_byte & 0x04) >> 2) + 1);
        // 40 sprites, without flitering
        for (int sprite_id = 39; sprite_id >= 0; sprite_id--)
        {
            uint16_t current_oam_address = OAM_TABLE_INITIAL_ADDDRESS + sprite_id * 4;
            uint8_t y_position = mem.get_memory_byte(current_oam_address);
            uint8_t x_position = mem.get_memory_byte(current_oam_address + 1);
            uint8_t tile_index = mem.get_memory_byte(current_oam_address + 2);
            tile_index++;
            //if (sprite_height == 16)
            //{
            //    // lsb of the sprite pattern number is ignored and treated as 0.
            //    tile_index &= 0xFE;
            //}
            //else
            //{
            //    tile_index &= 0xFF;
            //}
            uint8_t temp_attritube = mem.get_memory_byte(current_oam_address + 3);
            //bool attributes_priority = temp_attritube & 0x80;
            bool attributes_y_flip = temp_attritube & 0x40;
            bool attributes_x_flip = temp_attritube & 0x20;
            //bool attributes_palette_number = temp_attritube & 0x10;

            if (!(y_position | x_position))
            {
                continue;
            }

            int16_t sprite_y = y_position - 16; // y-coordinate offset: 0x10
            int16_t sprite_x = x_position - 8;  // x-coordinate offset: 0x08
            uint16_t line = line_number_y - sprite_y;
            // if not in this line, dont render
            if (line_number_y < sprite_y || line >= sprite_height)
            {
                continue;
            }
            if (sprite_x < -7 || sprite_x >= (SCREEN_WIDTH))
            {
                continue;
            }

            bool flip_x = attributes_x_flip;
            bool flip_y = attributes_y_flip;

            // actual position is attr_y -8
            line -= 8;

            if (flip_y)
            {
                // flip vertically
                line = sprite_height - line - 1;
            }

            uint16_t tile_location = 0x8000 + tile_index * 16 + line * 2;
            uint8_t sprite_tile_line_one = mem.get_memory_byte(tile_location);
            uint8_t sprite_tile_line_two = mem.get_memory_byte(tile_location + 1);

            for (int x = 0; x < PIXELS_PER_TILELINE; x++)
            {
                // if out of range, we dont need to render
                if (sprite_x + x < 0 || sprite_x + x >= (SCREEN_WIDTH))
                {
                    continue;
                }

                int pixel_x_in_line = PIXELS_PER_TILELINE - x % PIXELS_PER_TILELINE - 1;

                if (flip_x || flip_y)
                {
                    // flip horizontally
                    pixel_x_in_line = PIXELS_PER_TILELINE - pixel_x_in_line - 1;
                }

                int color = mix_tile_colors(pixel_x_in_line, sprite_tile_line_one, sprite_tile_line_two);

                if (!color)
                {
                    continue;
                }
                form.set_pixel_color(sprite_x + x, line_number_y, color, scale);
            }
        }
    }
}

void Ppu::update_lyc(Memory &mem)
{
    // get current LY and LYC and STAT
    uint8_t stat_byte = mem.get_memory_byte(STAT_ADDRESS);
    uint8_t ly_byte = mem.get_memory_byte(LY_ADDRESS);
    uint8_t lyc_byte = mem.get_memory_byte(LYC_ADDRESS);
    // determine whether LY==LYC
    if (ly_byte == lyc_byte)
    {
        // set Coincidence Flag (bit 2)
        stat_byte |= 0x04;
        if (stat_byte & 0x40)
        {
            uint8_t interrupt_flag_byte = mem.get_memory_byte(IF_ADDRESS);
            interrupt_flag_byte |= 0x02;
            mem.set_memory_byte(IF_ADDRESS, interrupt_flag_byte);
        }
    }
    else
    {
        // unset Coincidence Flag (bit 2)
        stat_byte &= 0xFB;
    }

    mem.set_memory_byte(STAT_ADDRESS, stat_byte);
}

void Ppu::add_time(int add_clocks)
{
    ppu_inner_clock += add_clocks;
}

void Ppu::reset_interrupt_registers(Memory &mem)
{
    // get current interrupt status
    uint8_t interrupt_flag_byte = mem.get_memory_byte(IF_ADDRESS);

    // change bit 0 (V-Blank) and bit 1 to 0 (LCDC Status)
    interrupt_flag_byte &= 0xFC;

    // write result to memory
    mem.set_memory_byte(IF_ADDRESS, interrupt_flag_byte);
}

uint8_t Ppu::mix_tile_colors(int bit, uint8_t tile_data_bytes_line_one, uint8_t tile_data_bytes_line_two)
{
    return (((tile_data_bytes_line_one >> bit) & 1) << 1) | ((tile_data_bytes_line_two >> bit) & 1);
}
