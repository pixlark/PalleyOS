/*
 * tio.c gives basic I/O interaction with the terminal
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <kstdio.h>
#include <kstdlib.h>
#include <io.h>
#include <tio.h>
#include <kheap.h>

#define internal static

// Video Buffer, points to mapped memory
Vga_Entry* platform_vb = (Vga_Entry*) 0xB8000;

typedef struct {
    Vga_Entry video_buffer[VB_SIZE];
} Video_Buffer_Page;

typedef struct {
    Video_Buffer_Page pages[FVB_PAGES];
    int num_pages;
} Video_Buffer;

typedef struct {
    int row;
    int height; 
    int width;
} VB_View;

typedef struct {
    int x;
    int y;
} Cursor;
Cursor cursor;

// Full Video Buffer
Video_Buffer fvb;

Video_Buffer_Page blank_buffer_page;

// Terminal View
VB_View term_view;
internal void vb_page_replace(Video_Buffer_Page* source, Video_Buffer_Page* dest);
internal void vb_shift_pages(Video_Buffer* vb);
internal void view_shift(Shift_Direction dir, uint32_t amt);
internal void render_screen();
internal void __render_screen(VB_View* view, Video_Buffer* fvb);
internal void render_cursor();
internal inline Vga_Entry create_vga_entry(unsigned char uc, Vga_Color color);

// Tio Helpers
internal void tio_write_char_color(char c, Vga_Color vc);

// Cursor helpers 
internal size_t cursor_get_index_in_buffer(Cursor* cursor);
internal size_t cursor_get_index(Cursor* cursor);
internal size_t view_get_buffer_end_index(VB_View* view);
internal size_t view_get_buffer_start_index(VB_View* view);
internal void platform_enable_cursor();

void tio_init() {
    
    // NOTE(alex): initializing Video Buffer to blank spaces
    Vga_Entry blank = create_vga_entry(' ', VGA_COLOR_BLACK);
    kmemset_u16(&blank_buffer_page, blank, sizeof(blank_buffer_page)/sizeof(blank_buffer_page.video_buffer[0]));
    
    fvb.num_pages = FVB_PAGES;
    for(int i = 0; i < FVB_PAGES; i++) {
        vb_page_replace(&blank_buffer_page, &fvb.pages[i]);
    }
    
    term_view.row = 0;
    term_view.height = TERM_HEIGHT;
    term_view.width = TERM_WIDTH;
    
    
    cursor.x = 0;
    cursor.y = 0;
    
}

internal inline Vga_Entry create_vga_entry(unsigned char uc, Vga_Color color) {
    return (uint16_t) uc | ((uint16_t) color) << 8;
}


internal void vb_page_replace(Video_Buffer_Page* source, Video_Buffer_Page* dest) {
    size_t num_elems = sizeof(dest->video_buffer);
    kmemcpy(dest->video_buffer, source->video_buffer, num_elems);
}

internal void vb_shift_pages(Video_Buffer* vb){
    if(vb->num_pages == 0){
        // TODO(alex): Not sure what to type here, need some sort of assert??
        return;
    }
    
    for(int i = 1; i < vb->num_pages; i++){
        vb_page_replace(&vb->pages[i], &vb->pages[i-1]);
    }
    vb_page_replace(&blank_buffer_page, &vb->pages[vb->num_pages-1]);
}

internal void __view_shift(VB_View* view, Video_Buffer* buff, Shift_Direction dir, uint32_t amt){
    for(uint32_t i = 0;
        i < amt; 
        i++) {
        
        if(dir == SHIFT_DIRECTION_UP) {
            view->row ++;
            cursor.y --;
            if(cursor.y < 0) cursor.y = 0;
            int num_rows = buff->num_pages * TERM_HEIGHT;
            if(view->row + view->height >= num_rows){
                // NOTE(alex): moves all data to previous page and makes last page blank.
                vb_shift_pages(buff);
                // NOTE(alex): Goes to second to last page
                view->row = buff->num_pages-2 * TERM_HEIGHT + 1;
            }
        }else {
            view->row --;
            cursor.y ++;
            if(view->row < 0) view->row = 0;
        }
    }
    render_screen();
    render_cursor();
}

internal void view_shift(Shift_Direction dir, uint32_t amt) {
    __view_shift(&term_view, &fvb, dir, amt);
}

void tio_shift_view(Shift_Direction dir, uint32_t amt) {
    view_shift(dir, amt);
}


/* TODO: This should be called whenever tryiing to write
 * at term_row, term_col. The screen could be shifted.
 * If that is the case, this will shift the screen so 
 * the user can see where they are typing
 */
internal void __cursor_inc(Cursor* cursor) {
    cursor->x++;
    if(cursor->x >= TERM_WIDTH) {
        cursor->y++;
        cursor->x= 0;
    }
    
    if(cursor->y >= TERM_HEIGHT-1) {
        view_shift(SHIFT_DIRECTION_UP, 1);
    }
    render_cursor(cursor);
}

void tio_cursor_inc() {
    __cursor_inc(&cursor);
}

internal void __cursor_dec(Cursor* cursor) {
    cursor->x--;
    if(cursor->x < 0){
        cursor->y--;
        cursor->x = TERM_WIDTH-1;
    }
    if(cursor->y < 0) cursor->y = 0;
    
    render_cursor(cursor);
}

void tio_cursor_dec() {
    __cursor_dec(&cursor);
}

inline internal void vb_set_value(Video_Buffer* buff, size_t index, Vga_Entry value){
    Vga_Entry* entry = (Vga_Entry*)(&buff->pages) + index;
    *entry = value;
}

inline internal Vga_Entry vb_get_value(Video_Buffer* buff, size_t index) {
    Vga_Entry* entry = ((Vga_Entry*)(&buff->pages) + index);
    return *entry;
}

internal void __tio_backspace(Cursor* cursor) {
    __cursor_dec(cursor);
    
    size_t cursor_buffer_index = cursor_get_index_in_buffer(cursor);
    size_t cursor_index = cursor_get_index(cursor);
    vb_set_value(&fvb, cursor_buffer_index, platform_vb[cursor_index]);
}

void tio_backspace() {
    __tio_backspace(&cursor);
}

// TODO(alex): Update this to work on an input line by line basis
// Shfits all characters visible to the right from the cursor and replaces
// it with a blank
void tio_shift_right(Cursor* cursor) {
    
    // TODO(alex): Update this to wrap words to next line
    size_t term_view_end_index = view_get_buffer_end_index(&term_view);
    size_t cursor_buffer_index = cursor_get_index_in_buffer(cursor);
    for(size_t j = term_view_end_index;
        j > cursor_buffer_index;
        j--) {
        vb_set_value(&fvb, j, vb_get_value(&fvb, j));
    }
    
    size_t cursor_index = cursor_get_index(cursor);
    vb_set_value(&fvb, cursor_buffer_index, platform_vb[cursor_index]);
    
    render_screen();
}

// TODO(alex): Update this to work on an input line by line basis
// Shfits all characters visible to the left from the cursor and replaces
// it with a blank
void tio_shift_left(Cursor* cursor) {
    size_t term_view_end_index = view_get_buffer_end_index(&term_view);
    size_t cursor_buffer_index = cursor_get_index_in_buffer(cursor);
    
    size_t fvb_index = cursor_buffer_index;
    if(cursor->x == 0) fvb_index += 1;
    
    // TODO(alex): Update this to shift until index line range end
    for(size_t j = fvb_index; j < term_view_end_index; j++)
        vb_set_value(&fvb, j, vb_get_value(&fvb, j+1));
    
    render_screen();
}

void tio_write_char(char c) {
    tio_write_char_color(c, VGA_COLOR_WHITE);
}

void tio_write_char_color(char c, Vga_Color vc){
    
    if(c == '\n') {
        cursor.y++;
        cursor.x = 0;
        if(cursor.y >= TERM_HEIGHT-1){
            view_shift(SHIFT_DIRECTION_UP, 1);
        }
        return;
    }else if(c == '\r'){
        cursor.x = 0;
        return;
    }
    
    // TODO(alex): Update to get cursor passed in
    size_t index = cursor_get_index(&cursor);
    Vga_Entry vga_entry = create_vga_entry(c, vc);
    platform_vb[index] = vga_entry; 
    
    // TODO(alex): Update this to set it correctly with guards
    size_t cursor_buffer_index = cursor_get_index_in_buffer(&cursor);
    vb_set_value(&fvb, cursor_buffer_index, vga_entry);
    
    tio_cursor_inc();
    render_cursor();
}

void tio_write(char* string) {
    tio_write_color(string, VGA_COLOR_WHITE); 
}

void tio_write_color(char* string, Vga_Color vc) {
    char* c = string;
    while(*c != '\0') {
        tio_write_char_color(*c, vc);
        c++;
    }
}

void tio_enable_cursor() {
    platform_enable_cursor();
}

internal void platform_enable_cursor()
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);
    
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}

internal void __render_cursor(Cursor* cursor)
{
    int16_t pos = cursor->y * TERM_WIDTH + cursor->x;
    if(pos >= VB_SIZE) return;
    
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t) (pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

internal void render_cursor() {
    __render_cursor(&cursor);
}

internal void __render_screen(VB_View* view, Video_Buffer* vb){
    size_t view_index = view_get_buffer_start_index(view);
    
    for(int i = 0; i < VB_SIZE; i++)
        platform_vb[i] = vb_get_value(vb, view_index+i);
}

internal void render_screen(){
    __render_screen(&term_view, &fvb);
}

internal size_t cursor_get_index_in_buffer(Cursor* cursor) {
    return (term_view.row+cursor->y) * TERM_WIDTH + cursor->x;
}

internal size_t cursor_get_index(Cursor* cursor) {
    return cursor->y * TERM_WIDTH + cursor->x;
}

internal size_t view_get_buffer_end_index(VB_View* view) {
    return (view->row + view->height) * TERM_WIDTH;
}

internal size_t view_get_buffer_start_index(VB_View* view) {
    return (view->row) * TERM_WIDTH;
}

