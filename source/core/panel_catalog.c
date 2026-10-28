#include "core/panel_catalog.h"

#include <stddef.h>

/*
 * Panel list from Marc Robledo's StreetPass Mii Plaza save editor
 * (streetpass-mii-plaza.js, v20170706, GPL-3.0). Names are only a fallback for
 * when no PNEL data is available.
 */
static const PanelInfo s_panels[PANEL_COUNT] = {
    {0, PANEL_STANDARD, 15, "Metroid: Other M"},
    {1, PANEL_STANDARD, 15, "Mario and Bowser"},
    {2, PANEL_STANDARD, 15, "Super Mario Galaxy 2"},
    {3, PANEL_STANDARD, 15, "The Legend of Zelda"},
    {4, PANEL_STANDARD, 15, "Kirby's Dream Land"},
    {5, PANEL_STANDARD, 15, "Pikmin"},
    {6, PANEL_STANDARD, 15, "New Super Mario Bros. Wii"},
    {7, PANEL_SPOTPASS, 24, "The Legend of Zelda: Ocarina of Time 3D"},
    {8, PANEL_SPOTPASS, 24, "Star Fox 64 3D"},
    {9, PANEL_SPOTPASS, 40, "Super Mario 3D Land"},
    {10, PANEL_SPOTPASS, 40, "Mario Kart 7"},
    {11, PANEL_SPOTPASS, 24, "Rhythm Heaven Fever"},
    {12, PANEL_SPOTPASS, 24, "Donkey Kong Country Returns"},
    {13, PANEL_SPOTPASS, 24, "Pilotwings Resort"},
    {14, PANEL_SPOTPASS, 40, "Kid Icarus: Uprising"},
    {15, PANEL_SPOTPASS, 40, "Fire Emblem Awakening"},
    {16, PANEL_SPOTPASS, 40, "Mario Tennis Open"},
    {17, PANEL_SPOTPASS, 40, "Kirby's 20th Anniversary"},
    {18, PANEL_SPOTPASS, 24, "Brain Age: Concentration Training"},
    {19, PANEL_SPOTPASS, 15, "All Nippon Airways"},
    {20, PANEL_SPOTPASS, 15, "Mac de DS Big Mac"},
    {21, PANEL_SPOTPASS, 40, "New Super Mario Bros. 2"},
    {22, PANEL_SPOTPASS, 40, "Kirby's Return to Dream Land"},
    {23, PANEL_SPOTPASS, 40, "Animal Crossing: New Leaf"},
    {24, PANEL_SPOTPASS, 40, "Luigi's Mansion: Dark Moon"},
    {25, PANEL_SPOTPASS, 40, "Dillon's Rolling Western: The Last Ranger"},
    {26, PANEL_SPOTPASS, 40, "Nintendo Starlets"},
    {27, PANEL_SPOTPASS, 40, "Xenoblade Chronicles"},
    {28, PANEL_SPOTPASS, 40, "New SUPER MARIO BROS. U + New SUPER LUIGI U"},
    {29, PANEL_SPOTPASS, 40, "The Legend of Zelda: Skyward Sword"},
    {30, PANEL_SPOTPASS, 40, "Mario & Luigi: Dream Team"},
    {31, PANEL_SPOTPASS, 40, "Chibi-Robo! Photo Finder"},
    {32, PANEL_SPOTPASS, 24, "Darumeshi Sports Store"},
    {33, PANEL_SPOTPASS, 40, "Pokémon X and Pokémon Y"},
    {34, PANEL_SPOTPASS, 40, "MONSTER HUNTER"},
    {35, PANEL_SPOTPASS, 40, "Kirby: Triple Deluxe"},
    {36, PANEL_SPOTPASS, 40, "SUPER MARIO 3D WORLD"},
    {37, PANEL_SPOTPASS, 40, "YOSHI'S New ISLAND"},
    {38, PANEL_SPOTPASS, 24, "Rusty's Real Deal Baseball"},
    {39, PANEL_SPOTPASS, 24, "Dalgu ne seupocheu yagu pyeon"},
    {40, PANEL_SPOTPASS, 15, "Nintendo Pocket Football Club"},
    {41, PANEL_SPOTPASS, 40, "Mega Man"},
    {42, PANEL_SPOTPASS, 24, "Kirby Fighters Deluxe/DeDeDe's Drum Dash Deluxe"},
    {43, PANEL_SPOTPASS, 40, "Daigasso! Band Brothers P"},
    {44, PANEL_SPOTPASS, 40, "FANTASY LIFE"},
    {45, PANEL_SPOTPASS, 15, "PIKMIN Short Movies 3D: The Night Juicer"},
    {46, PANEL_SPOTPASS, 15, "PIKMIN Short Movies 3D: Treasure in a Bottle"},
    {47, PANEL_SPOTPASS, 15, "PIKMIN Short Movies 3D: Occupational Hazards"},
    {48, PANEL_SPOTPASS, 40, "Sonic Boom"},
    {49, PANEL_SPOTPASS, 40, "Captain Toad: Treasure Tracker"},
    {50, PANEL_SPOTPASS, 40, "ULTIMATE NES REMIX"},
    {51, PANEL_SPOTPASS, 40, "Super Smash Bros. for 3DS / Wii U"},
    {52, PANEL_SPOTPASS, 40, "MONSTER HUNTER"},
    {53, PANEL_SPOTPASS, 24, "Tomodachi Life Friendship Fiesta"},
    {54, PANEL_SPOTPASS, 24, "Mario & Happy"},
    {55, PANEL_SPOTPASS, 15, "Picross 3D Round 2"},
    {56, PANEL_SPOTPASS, 40, "YO-KAI WATCH"},
    {57, PANEL_SPOTPASS, 15, "Mario & Luigi: Paper Jam"},
    {58, PANEL_SPOTPASS, 40, "Animal Crossing: Happy Home Designer"},
    {59, PANEL_SPOTPASS, 40, "FINAL FANTASY EXPLORERS"},
    {60, PANEL_SPOTPASS, 15, "Kirby: Planet Robobot"},
    {61, PANEL_SPOTPASS, 15, "Culdcept Revolt"},
    {62, PANEL_SPOTPASS, 15, "METROID PRIME FEDERATION FORCE"},
};

/* Names as in the RomFS of Mii Plaza EUR. */
static const char *const s_builtin_image[PANEL_STANDARD_COUNT] = {
    "item_panel_Samus.lz77",    "item_panel_MarioKoopa.lz77", "item_panel_Galaxy2.lz77",
    "item_panel_Link.lz77",     "item_panel_Kirby.lz77",      "item_panel_Pikmin.lz77",
    "item_panel_NewMario.lz77",
};

const char *panel_catalog_builtin_image(unsigned id)
{
    return id < PANEL_STANDARD_COUNT ? s_builtin_image[id] : NULL;
}

const PanelInfo *panel_catalog_get(unsigned id)
{
    if (id >= PANEL_COUNT)
        return NULL;
    return &s_panels[id];
}

unsigned panel_catalog_columns(unsigned piece_count)
{
    /*
     * Row by row from the top left is confirmed for 15 pieces; for 24 and 40
     * it is a hypothesis until confirmed on a console.
     */
    switch (piece_count) {
    case 15:
        return 5;
    case 24:
        return 6;
    case 40:
        return 8;
    default:
        return 0;
    }
}
