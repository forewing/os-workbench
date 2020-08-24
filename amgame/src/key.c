#include <game.h>
#include <key.h>

static int key_table[_KEY_PAGEDOWN + 1] = {}; // 0 for up, 1 for down

void read_key_game()
{
    // #define KEYNAME(key)
    //     [_KEY_##key] = #key,
    //     static const char *key_names[] = {_KEYS(KEYNAME)};
    int keycode = read_key();
    if (keycode != _KEY_NONE)
    {
        if (keycode & 0x8000)
        {
            if (!key_table[keycode & 0x7FFF])
            {
                // puts("Key down: ");
                key_table[keycode & 0x7FFF] = 1;
            }
            else
            {
                return;
            }
        }
        else
        {
            // puts("Key up: ");
            key_table[keycode & 0x7FFF] = 0;
        }
        // puts(key_names[keycode & 0x7FFF]);
        // puts("\n");
    }
    return;
}

int key_chk(int keycode)
{
    if (keycode > _KEY_PAGEDOWN)
    {
        return 0;
    }
    else
    {
        return key_table[keycode];
    }
}

void key_catch(int keycode)
{
    while (1)
    {
        read_key_game();
        if (key_chk(keycode))
        {
            return;
        }
    }
    return;
}

void key_catch_up(int keycode)
{
    while (1)
    {
        read_key_game();
        if (!key_chk(keycode))
        {
            return;
        }
    }
    return;
}

void key_catch_full(int keycode)
{
    key_catch(keycode);
    key_catch_up(keycode);
    return;
}