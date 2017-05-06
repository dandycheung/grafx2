/* vim:expandtab:ts=2 sw=2:
*/
/*  Grafx2 - The Ultimate 256-color bitmap paint program

    Copyright 2008      Franck Charlet
    Copyright 2007-2008 Adrien Destugues
    Copyright 1996-2001 Sunset Design (Guillaume Dorme & Karl Maritaud)

    Grafx2 is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; version 2
    of the License.

    Grafx2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Grafx2; if not, see <http://www.gnu.org/licenses/>

********************************************************************************

    Graphical interface management functions (windows, menu, cursor)
*/

#include <math.h>
#include <stdlib.h> // atoi()
#include <string.h> // strncpy() strlen()

#include "windows.h"

#include "engine.h"
#include "errors.h"
#include "global.h"
#include "graph.h"
#include "input.h"
#include "misc.h"
#include "op_c.h"
#include "readline.h"
#include "sdlscreen.h"
#include "palette.h"

T_Toolbar_button Buttons_Pool[NB_BUTTONS];
T_Menu_Bar Menu_bars[MENUBAR_COUNT] = 
  {{MENU_WIDTH,  9, 1, 45, {NULL,NULL,NULL},  20, BUTTON_HIDE }, // Status
  {MENU_WIDTH, 14, 1, 35, {NULL,NULL,NULL}, 236, BUTTON_ANIM_PLAY }, // Animation
  {MENU_WIDTH, 10, 1, 35, {NULL,NULL,NULL}, 144, BUTTON_LAYER_SELECT }, // Layers
  {MENU_WIDTH, 35, 1,  0, {NULL,NULL,NULL}, 254, BUTTON_CHOOSE_COL }} // Main
  ;


/// Width of one layer button, in pixels before scaling
word Layer_button_width = 1;

// L'encapsulation tente une perc�e...ou un dernier combat.

// Nombre de cellules r�el dans la palette du menu
word Menu_cells_X;
word Palette_cells_X()
{
  return Menu_cells_X;
}
word Menu_cells_Y;
word Palette_cells_Y()
{
  return Menu_cells_Y;
}

// Affichage d'un pixel dans le menu (si visible)
void Pixel_in_menu(word bar, word x, word y, byte color)
{
  if (Menu_is_visible && Menu_bars[bar].Visible)
    Block(x*Menu_factor_X,(y+Menu_bars[bar].Top)*Menu_factor_Y+Menu_Y,Menu_factor_X,Menu_factor_Y,color);
}

// Affichage d'un pixel dans le menu et met a jour la bitmap de skin
void Pixel_in_menu_and_skin(word bar, word x, word y, byte color)
{
  Pixel_in_menu(bar, x, y, color);
  Menu_bars[bar].Skin[2][y*Menu_bars[bar].Skin_width + x] = color;  
}

// Affichage d'un pixel dans la fen�tre (la fen�tre doit �tre visible)
void Pixel_in_window(word x,word y,byte color)
{
    Block((x*Menu_factor_X)+Window_pos_X,(y*Menu_factor_Y)+Window_pos_Y,Menu_factor_X,Menu_factor_Y,color);
}

// Affichage d'un rectangle dans la fen�tre (la fen�tre doit �tre visible)
void Window_rectangle(word x_pos,word y_pos,word width,word height,byte color)
{
  Block((x_pos*Menu_factor_X)+Window_pos_X,(y_pos*Menu_factor_Y)+Window_pos_Y,width*Menu_factor_X,height*Menu_factor_Y,color);
}


// -- Affichages de diff�rents cadres dans une fen�tre -----------------------

  // -- Frame g�n�ral avec couleurs param�trables --

void Window_display_frame_generic(word x_pos,word y_pos,word width,word height,
                                    byte color_tl,byte color_br,byte color_s,byte color_tlc,byte color_brc)
// Param�tres de couleurs:
// color_tl =Bords Haut et Gauche
// color_br =Bords Bas et Droite
// color_s  =Coins Haut-Droite et Bas-Gauche
// color_tlc=Coin Haut-Gauche
// color_brc=Coin Bas-Droite
{
  // Bord haut (sans les extr�mit�s)
  Window_rectangle(x_pos+1,y_pos,width-2,1,color_tl);

  // Bord bas (sans les extr�mit�s)
  Window_rectangle(x_pos+1,y_pos+height-1,width-2,1,color_br);

  // Bord gauche (sans les extr�mit�s)
  Window_rectangle(x_pos, y_pos+1,1,height-2,color_tl);

  // Bord droite (sans les extr�mit�s)
  Window_rectangle(x_pos+width-1,y_pos+1,1,height-2,color_br);

  // Coin haut gauche
  Pixel_in_window(x_pos,y_pos,color_tlc);
  // Coin haut droite
  Pixel_in_window(x_pos+width-1,y_pos,color_s);
  // Coin bas droite
  Pixel_in_window(x_pos+width-1,y_pos+height-1,color_brc);
  // Coin bas gauche
  Pixel_in_window(x_pos,y_pos+height-1,color_s);
}

  // -- Frame dont tout le contour est d'une seule couleur --

void Window_display_frame_mono(word x_pos,word y_pos,word width,word height,byte color)
{
  Window_display_frame_generic(x_pos,y_pos,width,height,color,color,color,color,color);
}

  // -- Frame creux: fonc� en haut-gauche et clair en bas-droite --

void Window_display_frame_in(word x_pos,word y_pos,word width,word height)
{
  Window_display_frame_generic(x_pos,y_pos,width,height,MC_Dark,MC_White,MC_Light,MC_Dark,MC_White);
}

  // -- Frame bomb�: clair en haut-gauche et fonc� en bas-droite --

void Window_display_frame_out(word x_pos,word y_pos,word width,word height)
{
  Window_display_frame_generic(x_pos,y_pos,width,height,MC_White,MC_Dark,MC_Light,MC_White,MC_Dark);
}

  // -- Frame de s�paration: un cadre bomb� dans un cadre creux (3D!!!) --

void Window_display_frame(word x_pos,word y_pos,word width,word height)
{
  Window_display_frame_in(x_pos,y_pos,width,height);
  Window_display_frame_out(x_pos+1,y_pos+1,width-2,height-2);
}


//-- Affichages relatifs � la palette dans le menu ---------------------------

  // -- Affichage des couleurs courante (fore/back) de pinceau dans le menu --

void Display_foreback(void)
{
  if (Menu_is_visible && Menu_bars[MENUBAR_TOOLS].Visible)
  {
    Block((MENU_WIDTH-17)*Menu_factor_X,Menu_Y+Menu_factor_Y,Menu_factor_X<<4,Menu_factor_Y*7,Back_color);
    Block((MENU_WIDTH-13)*Menu_factor_X,Menu_Y+(Menu_factor_Y<<1),Menu_factor_X<<3,Menu_factor_Y*5,Fore_color);

    Update_rect((MENU_WIDTH-17)*Menu_factor_X,Menu_Y+Menu_factor_Y,Menu_factor_X<<4,Menu_factor_Y*7);
  }
}

/*! Get the top left corner for the palette cell of a color
    @param index Index of the color, starting at 0 for the top left one. Limited to Menu_cells_X/Menu_cells_Y.
*/
word Palette_cell_X(byte index)
{
  if (Config.Palette_vertical)
  {
    return (MENU_WIDTH+1+((index-First_color_in_palette)%Menu_cells_X)*Menu_palette_cell_width)*Menu_factor_X;
  }
  else
  {
    return (MENU_WIDTH+1+((index-First_color_in_palette)/Menu_cells_Y)*Menu_palette_cell_width)*Menu_factor_X;
  }
}

/*! Get the top left corner for the palette cell of a color
    @param index Index of the color, starting at 0 for the top left one. Limited to Menu_cells_X/Menu_cells_Y.
*/
word Palette_cell_Y(byte index)
{
  if (Config.Palette_vertical)
  {
    return Menu_Y+((1+(((index-First_color_in_palette)/Menu_cells_X)*(Menu_bars[MENUBAR_TOOLS].Height/Menu_cells_Y)))*Menu_factor_Y);
  }
  else
  {
    return Menu_Y+((1+(((index-First_color_in_palette)%Menu_cells_Y)*(Menu_bars[MENUBAR_TOOLS].Height/Menu_cells_Y)))*Menu_factor_Y);
  }
}

void Set_fore_color(byte color)
{
  byte old_fore_color = Fore_color;
  
  Fore_color=color;
  Reposition_palette();
  Display_foreback();
  Frame_menu_color(old_fore_color);
  Frame_menu_color(Fore_color);
}

void Set_back_color(byte color)
{
  byte old_back_color = Back_color;
  
  Back_color=color;
  Display_foreback();
  Frame_menu_color(old_back_color);
  Frame_menu_color(Back_color);
}

///
/// Redraw the cell in the menu palette for ::Fore_color.
/// This function checks bounds, it won't draw anything if Fore_color is not visible.
/// @param id: Color number to frame
void Frame_menu_color(byte id)
{
  word start_x,start_y,end_x,end_y;
  word index;
  word cell_height=Menu_bars[MENUBAR_TOOLS].Height/Menu_cells_Y;
  byte color;
  
  if (! Menu_bars[MENUBAR_TOOLS].Visible)
    return;

  if (id==Fore_color)
    color = MC_White;
  else if (id==Back_color)
    color = MC_Dark;
  else
    color = MC_Black;

  if ((id>=First_color_in_palette) && (id<First_color_in_palette+Menu_cells_X*Menu_cells_Y) && (Menu_is_visible))
  {
    if (Config.Separate_colors)
    {
      start_x=Palette_cell_X(id)-1;
      start_y=Palette_cell_Y(id)-1;

      // TODO: if color is black, we are unselecting a color. If another color next to it is selected, we
      // will erase one edge of its selection square.
      // We should check for that here.
      // But we have to find which color is above and below (not so easy) and for the horizontal, check we
      // are not at the edge of the palette. This makes a lot of cases to handle.
      // Top
      Block(start_x,start_y,(Menu_palette_cell_width)*Menu_factor_X+1,1,color);
      // Bottom
      Block(start_x,start_y+cell_height*Menu_factor_Y,(Menu_palette_cell_width)*Menu_factor_X+1,1,color);

      // Left
      Block(start_x,start_y+1,1,(cell_height)* Menu_factor_Y,color);
      //Right
      Block(start_x+(Menu_palette_cell_width*Menu_factor_X),start_y+1,1,(cell_height)* Menu_factor_Y,color);

      Update_rect(start_x,start_y,(Menu_palette_cell_width+1)*Menu_factor_X,(cell_height+1)* Menu_factor_Y);
    }
    else
    {
      // Not separated colors
      start_x=Palette_cell_X(id);
      start_y=Palette_cell_Y(id);

      if (color==MC_Black)
      {
        int transparent = -1;
        int cw = (Menu_palette_cell_width)*Menu_factor_X;
        int ch = (cell_height)*Menu_factor_Y;

        if (Main_backups->Pages->Image_mode == 0 && Main_backups->Pages->Nb_layers > 1)
            transparent = Main_backups->Pages->Transparent_color;

        // Color is not selected, no dotted lines
        Block(start_x,start_y,Menu_palette_cell_width*Menu_factor_X,
            cell_height*Menu_factor_Y,id);
        if (id == transparent) {
            Block(start_x, start_y,
              cw / 2, ch / 2, MC_Light);
            Block(start_x + cw / 2, start_y + ch / 2,
              (cw+1) / 2, (ch+1) / 2, MC_Dark);
        }

        Update_rect(start_x,start_y,Menu_palette_cell_width*Menu_factor_X,cell_height*Menu_factor_Y);
      }
      else
      {
        end_x=Menu_palette_cell_width-1;
        end_y=cell_height-1;

        // Draw dotted lines

        // Top line
        for (index=0; index<=end_x; index++)
          Block(start_x+index*Menu_factor_X,start_y,
                Menu_factor_X,Menu_factor_Y,
                ((index)&1)?color:MC_Black);
        // Left line
        for (index=1; index<end_y; index++)
          Block(start_x,start_y+index*Menu_factor_Y,
                Menu_factor_X,Menu_factor_Y,
                ((index)&1)?color:MC_Black);
        // Right line
        for (index=1; index<end_y; index++)
          Block(start_x+end_x*Menu_factor_X,start_y+index*Menu_factor_Y,
                Menu_factor_X,Menu_factor_Y,
                ((index+end_x)&1)?color:MC_Black);
        // Bottom line
        for (index=0; index<=end_x; index++)
          Block(start_x+index*Menu_factor_X,start_y+end_y*Menu_factor_Y,
                Menu_factor_X,Menu_factor_Y,
                ((index+end_y)&1)?color:MC_Black);

        Update_rect(start_x*Menu_factor_X,start_y*Menu_factor_Y,Menu_palette_cell_width*Menu_factor_X,Menu_Y+cell_height*Menu_factor_Y);
      }
    }
  }
}

  // -- Afficher la palette dans le menu --

void Display_menu_palette(void)
{
  int color;
  byte cell_height=Menu_bars[MENUBAR_TOOLS].Height/Menu_cells_Y;
  // width: Menu_palette_cell_width

  if (Menu_is_visible && Menu_bars[MENUBAR_TOOLS].Visible)
  {
	int transparent = -1;
	int cw,ch;

	// Fill the whole palette area with black
    Block(
      Menu_bars[MENUBAR_TOOLS].Width*Menu_factor_X,
      Menu_Y,
      Screen_width-(Menu_bars[MENUBAR_TOOLS].Width*Menu_factor_X),
      (Menu_bars[MENUBAR_TOOLS].Height)*Menu_factor_Y,
      MC_Black);

	if (Main_backups->Pages->Image_mode == 0
		&& Main_backups->Pages->Nb_layers > 1)
		transparent = Main_backups->Pages->Transparent_color;

	// Compute the size of the color cells (they are smaller by 1px when using
	// 'separate colors"
    if (Config.Separate_colors) {
		cw = Menu_palette_cell_width * Menu_factor_X - 1;
        ch = cell_height * Menu_factor_Y - 1;
	} else {
		cw = (Menu_palette_cell_width)*Menu_factor_X;
        ch = (cell_height)*Menu_factor_Y;
	}

	for (color=First_color_in_palette;color<256&&(color-First_color_in_palette)<Menu_cells_X*Menu_cells_Y;color++) {
		// Draw the color block
        Block(Palette_cell_X(color), Palette_cell_Y(color), cw, ch, color);

		// Make the transparent color more visible by adding a MC_Dark/MC_Light pattern to it.
		if (color == transparent) {
        	Block(Palette_cell_X(color),
              Palette_cell_Y(color),
              cw / 2, ch / 2, MC_Light);
        	Block(Palette_cell_X(color) + cw / 2,
              Palette_cell_Y(color) + ch / 2,
              (cw+1) / 2, (ch+1) / 2, MC_Dark);
		}
	  }

    Frame_menu_color(Back_color);
    Frame_menu_color(Fore_color);
    Update_rect(MENU_WIDTH*Menu_factor_X,Menu_Y,Screen_width-(MENU_WIDTH*Menu_factor_X),(Menu_height-11)*Menu_factor_Y);
  }
}

  // -- Recalculer l'origine de la palette dans le menu pour rendre la
  //    Fore_color visible --

void Reposition_palette(void)
{
  byte old_color=First_color_in_palette;
  short cells;
  if (Config.Palette_vertical)
    cells=Menu_cells_X;
  else
    cells=Menu_cells_Y;
    

  if (Fore_color<First_color_in_palette)
  {
    while (Fore_color<First_color_in_palette)
      First_color_in_palette-=cells;
  }
  else
  {
    while (Fore_color>=First_color_in_palette+Menu_cells_X*Menu_cells_Y)
      First_color_in_palette+=cells;
  }
  if (old_color!=First_color_in_palette)
    Display_menu_palette();
}

void Change_palette_cells()
{
  // On initialise avec la configuration de l'utilisateur
  Menu_cells_X=Config.Palette_cells_X;
  Menu_cells_Y=Config.Palette_cells_Y;
  // Mais on sait jamais
  if (Menu_cells_X<1)
    Menu_cells_X=1;
  if (Menu_cells_Y<1)
    Menu_cells_Y=1;
  
  while (1)
  {
    Menu_palette_cell_width = ((Screen_width/Menu_factor_X)-(MENU_WIDTH+2)) / Menu_cells_X;

    // Si �a tient, c'est bon. Sinon, on retente avec une colonne de moins
    if (Menu_palette_cell_width>2)
      break;
    Menu_cells_X--;
  }
  
  // Cale First_color_in_palette sur un multiple du nombre de cellules (arrondi inf�rieur)
  if (Config.Palette_vertical)
    First_color_in_palette=First_color_in_palette/Menu_cells_X*Menu_cells_X;
  else
    First_color_in_palette=First_color_in_palette/Menu_cells_Y*Menu_cells_Y;

  // Si le nombre de cellules a beaucoup augment� et qu'on �tait pr�s de
  // la fin, il faut reculer First_color_in_palette pour montrer plein
  // de couleurs.
  if ((int)First_color_in_palette+(Menu_cells_Y)*Menu_cells_X*2>=256)
  {
    if (Config.Palette_vertical)
      First_color_in_palette=255/Menu_cells_X*Menu_cells_X-(Menu_cells_Y-1)*Menu_cells_X;
    else
      First_color_in_palette=255/Menu_cells_Y*Menu_cells_Y-(Menu_cells_X-1)*Menu_cells_Y;
  }
  
  // Mise � jour de la taille du bouton dans le menu. C'est pour pas que
  // la bordure noire soit active.
  Buttons_Pool[BUTTON_CHOOSE_COL].Width=(Menu_palette_cell_width*Menu_cells_X)-1;
  Buttons_Pool[BUTTON_CHOOSE_COL].Height=(MENU_HEIGHT-9)/Menu_cells_Y*Menu_cells_Y-1;
}

// Retrouve la couleur sur laquelle pointe le curseur souris.
// Cette fonction suppose qu'on a d�ja v�rifi� que le curseur est dans
// la zone rectangulaire du BUTTON_CHOOSE_COL
// La fonction renvoie -1 si on est "trop � gauche" (pas possible)
// ou apr�s la couleur 255 (Ce qui peut arriver si la palette est affich�e
// avec un nombre de lignes qui n'est pas une puissance de deux.)
int Pick_color_in_palette()
{
  int color;
  int line;
  int column;

  line=(((Mouse_Y-Menu_Y)/Menu_factor_Y)-1)/((Menu_bars[MENUBAR_TOOLS].Height)/Menu_cells_Y);
  column=(((Mouse_X/Menu_factor_X)-(MENU_WIDTH+1))/Menu_palette_cell_width);
  if (Config.Palette_vertical)
  {
    color=First_color_in_palette+line*Menu_cells_X+column;
  }
  else
  {
    color=First_color_in_palette+line+column*Menu_cells_Y;
  }
  if (color<0 || color>255)
    return -1;
  return color;
}

/// Draws a solid textured area, to the right of a toolbar.
void Draw_bar_remainder(word current_menu, word x_off)
{
  word y_pos;
  word x_pos;

  for (y_pos=0;y_pos<Menu_bars[current_menu].Height;y_pos++)
    for (x_pos=x_off;x_pos<Screen_width/Menu_factor_X;x_pos++)
      Pixel_in_menu(current_menu, x_pos, y_pos, Menu_bars[current_menu].Skin[0][y_pos * Menu_bars[current_menu].Skin_width + Menu_bars[current_menu].Skin_width - 2 + (x_pos&1)]);
}

            
/// Display / update the layer menubar
void Display_layerbar(void)
{
  
  if (Menu_bars[MENUBAR_LAYERS].Visible)
  {
    word x_off=0;
    word button_width = LAYER_SPRITE_WIDTH;
    word button_number = Main_backups->Pages->Nb_layers;
    word horiz_space;
    word current_button;
    word repeats=1;
    
    // Available space in pixels
    horiz_space = Screen_width / Menu_factor_X - Menu_bars[MENUBAR_LAYERS].Skin_width;
    
    // Don't display all buttons if not enough room
    if (horiz_space/button_width < button_number)
      button_number = horiz_space/button_width;
    // Only 16 icons at the moment
    if (button_number > 16) // can be different from MAX_NB_LAYERS
      button_number = 16;
  
    // Enlarge the buttons themselves if there's enough room
    while (button_number*(button_width+2) < horiz_space && repeats < 20)
    {
      repeats+=1;
      button_width+=2;
    }
    
    x_off=Menu_bars[MENUBAR_LAYERS].Skin_width;
    for (current_button=0; current_button<button_number; current_button++)
    {
      word x_pos=0;
      word y_pos;
      word sprite_index;
      
      if (Main_current_layer == current_button)
        sprite_index=1;
      else if (Main_layers_visible & (1 << current_button))
        sprite_index=0;
      else
        sprite_index=2;
      
      
      for (y_pos=0;y_pos<LAYER_SPRITE_HEIGHT;y_pos++)
      {
        word source_x=0;
        
        for (source_x=0;source_x<LAYER_SPRITE_WIDTH;source_x++)
        {
          short i = 1;
          
          // This stretches a button, by duplicating the 2nd from right column 
          // and 3rd column from left.    
          if (source_x == 1 || (source_x == LAYER_SPRITE_WIDTH-3))
            i=repeats;
          
          for (;i>0; i--)
          {
            Pixel_in_menu(MENUBAR_LAYERS, x_pos + x_off, y_pos, Gfx->Layer_sprite[sprite_index][current_button][y_pos][source_x]);
            x_pos++;
          }
        }
        // Next line
        x_pos=0;
      }    
      // Next button
      x_off+=button_width;
    }
    // Texture any remaining space to the right.
    // This overwrites any junk like deleted buttons.
    Draw_bar_remainder(MENUBAR_LAYERS, x_off);
    
    // Update the active area of the layers pseudo-button
    Buttons_Pool[BUTTON_LAYER_SELECT].Width = button_number * button_width;
    
    // Required to determine which layer button is clicked
    Layer_button_width = button_width;
    
    // A screen refresh required by some callers
    Update_rect(
      Menu_bars[MENUBAR_LAYERS].Skin_width, 
      Menu_Y+Menu_bars[MENUBAR_LAYERS].Top*Menu_factor_Y, 
      horiz_space*Menu_factor_X, 
      Menu_bars[MENUBAR_LAYERS].Height*Menu_factor_Y);
  }
  if (Menu_bars[MENUBAR_ANIMATION].Visible)
  {
    char str[9];
    // Rest of horizontal line
    Draw_bar_remainder(MENUBAR_ANIMATION, Menu_bars[MENUBAR_ANIMATION].Skin_width);
    // Frame# background rectangle
    // Block((Menu_bars[MENUBAR_ANIMATION].Skin_width)*Menu_factor_X,(0+Menu_bars[MENUBAR_ANIMATION].Top)*Menu_factor_Y+Menu_Y,8*8*Menu_factor_X,8*Menu_factor_Y,MC_Light);
    // Frame #/#
    snprintf(str, 8, "%3d/%3d", Main_current_layer+1, Main_backups->Pages->Nb_layers);
    Print_general((59)*Menu_factor_X,(Menu_bars[MENUBAR_ANIMATION].Top+3)*Menu_factor_Y+Menu_Y,str,MC_Black,MC_Light);
    Update_rect(
      (59)*Menu_factor_X,
      (Menu_bars[MENUBAR_ANIMATION].Top+3)*Menu_factor_Y+Menu_Y,
      7*8*Menu_factor_X, 
      8*Menu_factor_Y);
  }
}


/// Display the whole menu
void Display_menu(void)
{
  word x_pos;
  word y_pos;
  int8_t current_menu;
  char str[4];


  if (Menu_is_visible)
  {
    // display menu sprite
    for (current_menu = MENUBAR_COUNT - 1; current_menu >= 0; current_menu --)
    {
      if(Menu_bars[current_menu].Visible)
      {
        // Skinned area
        for (y_pos=0;y_pos<Menu_bars[current_menu].Height;y_pos++)
          for (x_pos=0;x_pos<Menu_bars[current_menu].Skin_width;x_pos++)
            Pixel_in_menu(current_menu, x_pos, y_pos, Menu_bars[current_menu].Skin[2][y_pos * Menu_bars[current_menu].Skin_width + x_pos]);

        if (current_menu == MENUBAR_LAYERS || current_menu == MENUBAR_ANIMATION)
        {
          // The layerbar has its own display, for the whole length.
          Display_layerbar();
        }
        else
        {
          // If some area is remaining to the right, texture it with a copy of
          // the last two columns
          Draw_bar_remainder(current_menu, Menu_bars[current_menu].Skin_width);
        }
        
        // Next bar
      }
    }

    // Display palette
    Display_menu_palette();

    // Display selected colors
    Display_foreback();


    if (!Windows_open)
    {
      if ((Mouse_Y<Menu_Y) && // Mouse in the picture area
          ( (!Main_magnifier_mode) || (Mouse_X<Main_separator_position) || (Mouse_X>=Main_X_zoom) ))
      {
        // Prepare display of XY coordinates even if in some cases they will be
        // erased with some other text
        if ( (Current_operation!=OPERATION_COLORPICK)
          && (Current_operation!=OPERATION_REPLACE) )
          Print_in_menu("X:       Y:             ",0);
        else
        {
          // The colorpicker display the color id between the parentheses
          Print_in_menu("X:       Y:       (    )",0);
          Num2str(Colorpicker_color,str,3);
          Print_in_menu(str,20);
          Print_general(170*Menu_factor_X,Menu_status_Y," ",0,Colorpicker_color);
        }
        Print_coordinates();
      }
      Print_filename();
    }
    // Now update the area: menu height and whole screen width (including palette)
    Update_rect(0,Menu_Y,Screen_width,Menu_height*Menu_factor_Y);
  }
}

// -- Affichage de texte -----------------------------------------------------

  // -- Afficher une cha�ne n'importe o� � l'�cran --

void Print_general(short x,short y,const char * str,byte text_color,byte background_color)
{
  word  index;
  int x_pos;
  int y_pos;
  byte *font_pixel;
  short real_x;
  short real_y;
  byte repeat_menu_x_factor;
  byte repeat_menu_y_factor;

  real_y=y;
  for (y_pos=0;y_pos<8<<3;y_pos+=1<<3)
  {
    real_x=0; // Position dans le buffer
    for (index=0;str[index]!='\0';index++)
    {
      // Pointeur sur le premier pixel du caract�re
      font_pixel=Menu_font+(((unsigned char)str[index])<<6);
      for (x_pos=0;x_pos<8;x_pos+=1)
        for (repeat_menu_x_factor=0;repeat_menu_x_factor<Menu_factor_X*Pixel_width;repeat_menu_x_factor++)
          Horizontal_line_buffer[real_x++]=*(font_pixel+x_pos+y_pos)?text_color:background_color;
    }
    for (repeat_menu_y_factor=0;repeat_menu_y_factor<Menu_factor_Y;repeat_menu_y_factor++)
      Display_line_fast(x,real_y++,index*Menu_factor_X*8,Horizontal_line_buffer);
  }
}

/// Draws a char in a window
void Print_char_in_window(short x_pos,short y_pos,const unsigned char c,byte text_color,byte background_color)
{
  short x,y;
  byte *pixel;
  // Premier pixel du caract�re
  pixel=Menu_font + (c<<6);
  
  for (y=0;y<8;y++)
    for (x=0;x<8;x++)
      Pixel_in_window(x_pos+x, y_pos+y,
            (*(pixel++)?text_color:background_color));
}

///Draws a char in a window, checking for bounds
void Print_in_window_limited(short x,short y,const char * str,byte size,byte text_color,byte background_color)
{
  char display_string[256];
  strncpy(display_string, str, size);
  display_string[size]='\0';

  if (strlen(str) > size)
  {
    display_string[size-1]=ELLIPSIS_CHARACTER;
  }
  Print_in_window(x, y, display_string, text_color, background_color);
}

/// Draws a string in a window
void Print_in_window(short x,short y,const char * str,byte text_color,byte background_color)
{
  short x_pos = x;
  int index;

  for (index=0;str[index]!='\0';index++)
  {
    Print_char_in_window(x,y,str[index],text_color,background_color);
    x+=8;
  }
  Update_window_area(x_pos,y,8*strlen(str),8);
}

// Draws a string in the menu's status bar
void Print_in_menu(const char * str, short position)
{
  Print_general((18+(position<<3))*Menu_factor_X,Menu_status_Y,str,MC_Black,MC_Light);
  Update_status_line(position, strlen(str));
}

/// Draws the mouse coordinates on the menu
/// Only update the digits and doesn't refresh the "X: Y:" labels. This function needs to be fast as it is called each time the mouse moves.
void Print_coordinates(void)
{
  char temp[5];

  if (Menu_is_visible && !Cursor_in_menu)
  {
    if ( (Current_operation==OPERATION_COLORPICK)
      || (Current_operation==OPERATION_RMB_COLORPICK)
      || (Current_operation==OPERATION_REPLACE) )
    {
      if ( (Paintbrush_X>=0) && (Paintbrush_Y>=0)
        && (Paintbrush_X<Main_image_width)
        && (Paintbrush_Y<Main_image_height) )
        Colorpicker_color=Read_pixel_from_current_screen(Paintbrush_X,Paintbrush_Y);
      else
        Colorpicker_color=0;
      Colorpicker_X=Paintbrush_X;
      Colorpicker_Y=Paintbrush_Y;

      Num2str(Colorpicker_color,temp,3);
      Print_in_menu(temp,20);
      Print_general(170*Menu_factor_X,Menu_status_Y," ",0,Colorpicker_color);
    }

    Num2str((dword)Paintbrush_X,temp,4);
    Print_in_menu(temp,2);
    Num2str((dword)Paintbrush_Y,temp,4);
    Print_in_menu(temp,11);
  }
}

  // -- Afficher le nom du fichier dans le menu --

void Print_filename(void)
{
  word max_size;
  word string_size;
  char display_string[256];

  // Determine maximum size, in characters
  max_size = 12 + (Screen_width / Menu_factor_X - 320) / 8;
  
  // Partial copy of the name
  {
#ifdef ENABLE_FILENAMES_ICONV
    char * input = Main_backups->Pages->Filename;
    size_t inbytesleft = strlen(input);
    char * output = display_string;
    size_t outbytesleft = sizeof(display_string)-1;
    if(cd != (iconv_t)-1 && (ssize_t)iconv(cd, &input, &inbytesleft, &output, &outbytesleft) >= 0)
      *output = '\0';
    else
#endif /* ENABLE_FILENAMES_ICONV */
    {
      strncpy(display_string, Main_backups->Pages->Filename, sizeof(display_string)-1);
      display_string[sizeof(display_string)-1] = '\0';
    }
  }
  string_size = strlen(display_string);
  display_string[max_size]='\0';

  if (string_size > max_size)
  {
    string_size = max_size;
    display_string[string_size-1]=ELLIPSIS_CHARACTER;
  }
  // Erase whole area
  Block(Screen_width-max_size*8*Menu_factor_X,
    Menu_status_Y,Menu_factor_X*max_size*8,Menu_factor_Y<<3,MC_Light);
  // Print
  Print_general(Screen_width-(string_size<<3)*Menu_factor_X,Menu_status_Y,display_string,MC_Black,MC_Light);
}

// Fonction d'affichage d'une chaine num�rique avec une fonte tr�s fine
// Sp�cialis�e pour les compteurs RGB
void Print_counter(short x,short y,const char * str,byte text_color,byte background_color)
{
  // Macros pour �crire des litteraux binaires.
  // Ex: Ob(11110000) == 0xF0
  #define Ob(x)  ((unsigned)Ob_(0 ## x ## uL))
  #define Ob_(x) ((x & 1) | (x >> 2 & 2) | (x >> 4 & 4) | (x >> 6 & 8) |                \
          (x >> 8 & 16) | (x >> 10 & 32) | (x >> 12 & 64) | (x >> 14 & 128))

  byte thin_font[14][8] = {
   { // 0
    Ob(00011100),
    Ob(00110110),
    Ob(00110110),
    Ob(00110110),
    Ob(00110110),
    Ob(00110110),
    Ob(00110110),
    Ob(00011100)
   },
   { // 1
    Ob(00001100),
    Ob(00011100),
    Ob(00111100),
    Ob(00001100),
    Ob(00001100),
    Ob(00001100),
    Ob(00001100),
    Ob(00001100)
   },
   { // 2
    Ob(00011100),
    Ob(00110110),
    Ob(00000110),
    Ob(00000110),
    Ob(00000110),
    Ob(00001100),
    Ob(00011000),
    Ob(00111110)
   },
   { // 3
    Ob(00011100),
    Ob(00110110),
    Ob(00000110),
    Ob(00001100),
    Ob(00000110),
    Ob(00000110),
    Ob(00110110),
    Ob(00011100)
   },
   { // 4
    Ob(00001100),
    Ob(00001100),
    Ob(00011000),
    Ob(00011000),
    Ob(00110000),
    Ob(00110100),
    Ob(00111110),
    Ob(00000100)
   },
   { // 5
    Ob(00111110),
    Ob(00110000),
    Ob(00110000),
    Ob(00111100),
    Ob(00000110),
    Ob(00000110),
    Ob(00110110),
    Ob(00011100)
   },
   { // 6
    Ob(00011100),
    Ob(00110110),
    Ob(00110000),
    Ob(00111100),
    Ob(00110110),
    Ob(00110110),
    Ob(00110110),
    Ob(00011100)
   },
   { // 7
    Ob(00111110),
    Ob(00000110),
    Ob(00000110),
    Ob(00001100),
    Ob(00011000),
    Ob(00011000),
    Ob(00011000),
    Ob(00011000)
   },
   { // 8
    Ob(00011100),
    Ob(00110110),
    Ob(00110110),
    Ob(00011100),
    Ob(00110110),
    Ob(00110110),
    Ob(00110110),
    Ob(00011100)
   },
   { // 9
    Ob(00011100),
    Ob(00110110),
    Ob(00110110),
    Ob(00011110),
    Ob(00000110),
    Ob(00000110),
    Ob(00110110),
    Ob(00011100)
   },
   { // (espace)
    Ob(00000000),
    Ob(00000000),
    Ob(00000000),
    Ob(00000000),
    Ob(00000000),
    Ob(00000000),
    Ob(00000000),
    Ob(00000000)
   },
   { // +
    Ob(00000000),
    Ob(00001000),
    Ob(00001000),
    Ob(00111110),
    Ob(00001000),
    Ob(00001000),
    Ob(00000000),
    Ob(00000000)
   },
   { // -
    Ob(00000000),
    Ob(00000000),
    Ob(00000000),
    Ob(00111110),
    Ob(00000000),
    Ob(00000000),
    Ob(00000000),
    Ob(00000000)
   },
   { // +-
    Ob(00001000),
    Ob(00001000),
    Ob(00111110),
    Ob(00001000),
    Ob(00001000),
    Ob(00000000),
    Ob(00111110),
    Ob(00000000)
   } };

  word  index;
  short x_pos;
  short y_pos;
  for (index=0;str[index]!='\0';index++)
  {
    int char_number;
    switch(str[index])
    {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        char_number=str[index]-'0';
        break;
      case ' ':
      default:
        char_number=10;
        break;
      case '+':
        char_number=11;
        break;
      case '-':
        char_number=12;
        break;
      case '�':
        char_number=13;
        break;
    }
    for (y_pos=0;y_pos<8;y_pos++)
    {
      for (x_pos=0;x_pos<6;x_pos++)
      {
        byte color = (thin_font[char_number][y_pos] & (1 << (6-x_pos))) ? text_color:background_color;
        Pixel_in_window(x+(index*6+x_pos),y+y_pos,color);
      }
    }
  }
  Update_window_area(x,y,strlen(str)*6,8);
}



///
/// Window asking for confirmation before an action is performed.
/// This function is able to display multi-line messages and
/// center the lines, but the carriage returns have to be explicit.
/// The function will clip the message in case of problem.
/// @return 1 if user pressed OK, 0 if CANCEL
byte Confirmation_box(char * message)
{
  short clicked_button;
  word  window_width = 120;
  word  nb_lines = 1;
  const char *c = message;
  short current_length=0;
  short current_line;

  // Count lines, and measure max line length
  for (c=message; *c != '\0'; c++)
  {
    if (*c == '\n')
    {
      current_length=0;
      nb_lines++;
    }
    else
    {
      current_length++;
      window_width=Max(window_width, (current_length<<3)+20);
    }
  }
  // Safety
  if (window_width>310)
    window_width=310;

  Open_window(window_width,52+(nb_lines<<3),"Confirmation");

  c=message;
  for (current_line=0; current_line < nb_lines; current_line++)
  {
    char * next_eol;
    char display_string[36+1];
    
    next_eol = strchr(c, '\n');
    if (next_eol==NULL) // last line
      current_length = strlen(c);
    else
      current_length = next_eol-c;

    // Safeguard
    if (current_length>36)
      current_length=36;
    // Copy part of string in null-terminated buffer
    strncpy(display_string, c, current_length);
    display_string[current_length]='\0';

    Print_in_window((window_width>>1)-(current_length<<2), 20+(current_line<<3), display_string, MC_Black, MC_Light);
    
    c += current_length;
    if (*c == '\n')
      c++;
  }

  Window_set_normal_button((window_width/3)-20     ,29+(nb_lines<<3),40,14,"Yes",1,1,SDLK_y); // 1
  Window_set_normal_button(((window_width<<1)/3)-20,29+(nb_lines<<3),40,14,"No" ,1,1,SDLK_n); // 2

  Update_window_area(0, 0, Window_width, Window_height);

  Display_cursor();

  do
  {
    clicked_button=Window_clicked_button();
    if (Key==SDLK_RETURN) clicked_button=1;
    if (Key==KEY_ESC) clicked_button=2;
  }
  while (clicked_button<=0);
  Key=0;

  Close_window();
  Display_cursor();

  return (clicked_button==1)? 1 : 0;
}


/// Window that allows you to enter a single value
int Requester_window(char* message, int initial_value)
{
  short clicked_button = 0;
  word window_width;
  char str[10];

  window_width=(strlen(message)<<3)+20;

  if (window_width<120)
    window_width = 120;

  Open_window(window_width, 60, "Request");

  Print_in_window((window_width>>1)-(strlen(message)<<2), 20, message,
    MC_Black, MC_Light);
  sprintf(str, "%d", initial_value);
  Window_set_input_button(10, 37, 4); // 1
  Print_in_window(11, 39, str, MC_Black, MC_Light);
  Window_set_normal_button(60 ,37,40,14,"OK",1,1,SDLK_y); // 2
  Window_set_normal_button(130,37,60,14,"Cancel" ,1,1,SDLK_n); // 3

  Update_window_area(0, 0, window_width, 60);
  Display_cursor();

  do
  {
    clicked_button = Window_clicked_button();
    if (clicked_button == 1)
      Readline(11, 39, str, 4, INPUT_TYPE_INTEGER);
    if (Key == SDLK_ESCAPE) clicked_button = 2;
  }
  while (clicked_button <= 0);

  Key = 0;

  Close_window();
  Display_cursor();

  return clicked_button==2?-1:atoi(str);
}


/// Window that show a warning message and wait for a click on the OK button
void Warning_message(char * message)
{
  short clicked_button;
  word  window_width;

  window_width=(strlen(message)<<3)+20;
  if (window_width<120)
    window_width=120;

  Open_window(window_width,60,"Warning!");

  Print_in_window((window_width>>1)-(strlen(message)<<2),20,message,MC_Black,MC_Light);
  Window_set_normal_button((window_width>>1)-20     ,37,40,14,"OK",1,1,SDLK_RETURN); // 1
  Update_window_area(0,0,window_width,60);
  Display_cursor();

  do
    clicked_button=Window_clicked_button();
  while ((clicked_button<=0) && (Key!=KEY_ESC) && (Key!=SDLK_o));
  Key=0;

  Close_window();
  Display_cursor();
}

/// Window that shows a big message (up to 35x13), and waits for a click on OK.
/// On call: Cursor must be displayed
/// On exit: Cursor is displayed
void Verbose_message(const char *caption, const char * message )
{
  short clicked_button;
  int line;
  int last_space;
  int nb_char;
  char buffer[36];
  byte original_cursor_shape = Cursor_shape;

  
  Open_window(300,160,caption);
  
  // Word-wrap the message
  for (line=0; line < 13 && *message!='\0'; line++)
  {
    last_space = -1;
    for (nb_char=0; nb_char<35 && message[nb_char]!='\0'; nb_char++)
    {
      buffer[nb_char]=message[nb_char];
      if (message[nb_char] == ' ')
      {
        last_space = nb_char;
      }
      else if (message[nb_char] == '\n')
      {
        last_space = nb_char;
        break;
      }
    }
    // Close line buffer
    if (message[nb_char]=='\0' || last_space == -1)
      last_space = nb_char;
    buffer[last_space]='\0';
    
    // Print
    Print_in_window(10,20+line*8,buffer,MC_Black,MC_Light);
    
    // Next line
    message=message+last_space;
    // Strip at most one carriage return and any leading spaces
    if (*message == '\n')
      message++;
    while (*message == ' ')
      message++;
  }

  Window_set_normal_button(300/2-20,160-23,40,14,"OK",1,1,SDLK_RETURN); // 1
  Update_window_area(0,0,Window_width,Window_height);
  Cursor_shape=CURSOR_SHAPE_ARROW;
  Display_cursor();

  do
    clicked_button=Window_clicked_button();
  while ((clicked_button<=0) && (Key!=KEY_ESC) && (Key!=SDLK_o));
  Key=0;

  Close_window();
  Cursor_shape=original_cursor_shape;
  Display_cursor();
}

  // -- Redessiner le sprite d'un bouton dans le menu --

void Display_sprite_in_menu(int btn_number,char sprite_number)
{
  Buttons_Pool[btn_number].Icon=sprite_number;

  if (Buttons_Pool[btn_number].Shape == BUTTON_SHAPE_TRIANGLE_TOP_LEFT)
    Buttons_Pool[btn_number+1].Icon=sprite_number;

  else if (Buttons_Pool[btn_number].Shape == BUTTON_SHAPE_TRIANGLE_BOTTOM_RIGHT)
    Buttons_Pool[btn_number-1].Icon=sprite_number;
}

  // -- Redessiner la forme du pinceau dans le menu --

void Display_paintbrush_in_menu(void)
{
  switch(Paintbrush_shape)
  {
    case PAINTBRUSH_SHAPE_COLOR_BRUSH:
      Display_sprite_in_menu(BUTTON_PAINTBRUSHES, MENU_SPRITE_COLOR_BRUSH);
      break;
    case PAINTBRUSH_SHAPE_MONO_BRUSH:
      Display_sprite_in_menu(BUTTON_PAINTBRUSHES, MENU_SPRITE_MONO_BRUSH);
      break;
    default:
      Display_sprite_in_menu(BUTTON_PAINTBRUSHES, -1);
      break;
  }
  Draw_menu_button(BUTTON_PAINTBRUSHES,BUTTON_RELEASED);
}

  // -- Dessiner un pinceau pr�d�fini dans la fen�tre --

void Display_paintbrush_in_window(word x,word y,int number)
  // Pinceau = 0..NB_PAINTBRUSH_SPRITES-1 : Pinceau pr�d�fini
{
  word x_pos;
  word y_pos;
  word window_x_pos;
  word window_y_pos;
  int x_size;
  int y_size;
  word origin_x;
  word origin_y;
  word width;
  word height;

  x_size=Menu_factor_X/Pixel_height;
  if (x_size<1)
    x_size=1;
  y_size=Menu_factor_Y/Pixel_width;
  if (y_size<1)
    y_size=1;

  width=Min(Paintbrush[number].Width,PAINTBRUSH_WIDTH);
  height=Min(Paintbrush[number].Height,PAINTBRUSH_WIDTH);
  
  origin_x = (x + 8)*Menu_factor_X - (width/2)*x_size+Window_pos_X;
  origin_y = (y + 8)*Menu_factor_Y - (height/2)*y_size+Window_pos_Y;

  for (window_y_pos=0,y_pos=0; y_pos<height; window_y_pos++,y_pos++)
    for (window_x_pos=0,x_pos=0; x_pos<width; window_x_pos++,x_pos++)
      if (Paintbrush[number].Sprite[y_pos][x_pos])
        Block(origin_x+window_x_pos*x_size,origin_y+window_y_pos*y_size,x_size,y_size,MC_Black);
  // On n'utilise pas Pixel_in_window() car on ne dessine pas
  // forc�ment avec la m�me taille de pixel.

  Update_rect( ToWinX(origin_x), ToWinY(origin_y),
        ToWinL(Paintbrush[number].Width),
        ToWinH(Paintbrush[number].Height)
  );
}

  // -- Dessiner des zigouigouis --

void Draw_thingumajig(word x,word y, byte color, short direction)
{
  word i;

  for (i=0; i<11; i++) Pixel_in_window(x,y+i,color);
  x+=direction;
  for (i=1; i<10; i++) Pixel_in_window(x,y+i,color);
  x+=direction+direction;
  for (i=3; i<8; i++) Pixel_in_window(x,y+i,color);
  x+=direction+direction;
  Pixel_in_window(x,y+5,color);
}

  // -- Dessiner un bloc de couleurs d�grad� verticalement

void Display_grad_block_in_window(word x_pos,word y_pos,word width,word height,word block_start,word block_end)
{
  word total_lines  =Menu_factor_Y*height;
  word nb_colors   =(block_start<=block_end)?block_end-block_start+1:block_start-block_end+1;
  word Selected_line_mode=(block_start<=block_end)?0:total_lines-1;

  word start_x       =Window_pos_X+(Menu_factor_X*x_pos);
  word line_width =Menu_factor_X*width;

  word start_y       =Window_pos_Y+(Menu_factor_Y*y_pos);
  word end_y         =start_y+total_lines;
  word index;

  if (block_start>block_end)
  {
    index=block_start;
    block_start=block_end;
    block_end=index;
  }

  for (index=start_y;index<end_y;index++,Selected_line_mode++)
    Block(start_x,index,line_width,1,block_start+(nb_colors*Selected_line_mode)/total_lines);

  Update_rect(ToWinX(x_pos),ToWinY(y_pos),line_width,total_lines);
}



  // -- Dessiner un petit sprite repr�sentant le type d'un drive --

void Window_display_icon_sprite(word x_pos,word y_pos,byte type)
{
  word i,j;

  for (j=0; j<ICON_SPRITE_HEIGHT; j++)
    for (i=0; i<ICON_SPRITE_WIDTH; i++)
      Pixel_in_window(x_pos+i,y_pos+j,Gfx->Icon_sprite[type][j][i]);
  Update_rect(ToWinX(x_pos),ToWinY(y_pos),ToWinL(ICON_SPRITE_WIDTH),ToWinH(ICON_SPRITE_HEIGHT));
}



void Display_menu_palette_avoiding_window(byte * table)
{
  // On part du principe qu'il n'y a que le bas d'une fen�tre qui puisse
  // empi�ter sur la palette... Et c'est d�j� pas mal!
  word color,real_color;
  word start_x,start_y;
  word end_x,end_y;
  word width;
  word height;
  word corner_x=Window_pos_X+Window_width*Menu_factor_X; // |_ Coin bas-droit
  word corner_y=Window_pos_Y+Window_height*Menu_factor_Y; // |  de la fen�tre +1

  if (Config.Separate_colors)
  {
    width=(Menu_palette_cell_width-1)*Menu_factor_X;
    height=Menu_factor_Y*((Menu_height-11)/Menu_cells_Y-1);
  }
  else
  {
    width=Menu_palette_cell_width*Menu_factor_X;
    height=Menu_factor_Y*((Menu_height-11)/Menu_cells_Y);
  }

  for (color=0,real_color=First_color_in_palette;color<Menu_cells_X*Menu_cells_Y;color++,real_color++)
  {
    if (table[real_color]!=real_color)
    {
      start_x=Palette_cell_X(real_color);
      start_y=Palette_cell_Y(real_color);
      end_x=start_x+width;
      end_y=start_y+height;

      //   On affiche le bloc en entier si on peut, sinon on le d�coupe autour
      // de la fen�tre.
      if ( (start_y>=corner_y) || (end_x<=Window_pos_X) || (start_x>=corner_x) )
        Block(start_x,start_y,width,height,real_color);
      else
      {

        if (start_x>=Window_pos_X)
        {
          if ( (end_x>corner_x) || (end_y>corner_y) )
          {
            if ( (end_x>corner_x) && (end_y>corner_y) )
            {
              Block(corner_x,start_y,end_x-corner_x,corner_y-start_y,real_color);
              Block(start_x,corner_y,width,end_y-corner_y,real_color);
            }
            else
            {
              if (end_y>corner_y)
                Block(start_x,corner_y,width,end_y-corner_y,real_color);
              else
                Block(corner_x,start_y,end_x-corner_x,height,real_color);
            }
          }
        }
        else
        {
          if (end_x<corner_x)
          {
            if (end_y>corner_y)
            {
              Block(start_x,start_y,Window_pos_X-start_x,corner_y-start_y,real_color);
              Block(start_x,corner_y,width,end_y-corner_y,real_color);
            }
            else
              Block(start_x,start_y,Window_pos_X-start_x,height,real_color);
          }
          else
          {
            if (end_y>corner_y)
            {
              Block(start_x,start_y,Window_pos_X-start_x,corner_y-start_y,real_color);
              Block(corner_x,start_y,end_x-corner_x,corner_y-start_y,real_color);
              Block(start_x,corner_y,width,end_y-corner_y,real_color);
            }
            else
            {
              Block(start_x,start_y,Window_pos_X-start_x,height,real_color);
              Block(corner_x,start_y,end_x-corner_x,height,real_color);
            }
          }
        }
      }
      {
        // Affichage du bloc directement dans le "buffer de fond" de la fenetre.
        // Cela permet au bloc de couleur d'apparaitre si on d�place la fenetre.
        short x_pos;
        short y_pos;
        short relative_x; // besoin d'une variable sign�e
        short relative_y; // besoin d'une variable sign�e
        // Attention aux unit�s
        relative_x = ((short)start_x - (short)Window_pos_X);
        relative_y = ((short)start_y - (short)Window_pos_Y);

        for (y_pos=relative_y;y_pos<(relative_y+height)&&y_pos<Window_height*Menu_factor_Y;y_pos++)
          for (x_pos=relative_x;x_pos<(relative_x+width)&&x_pos<Window_width*Menu_factor_X;x_pos++)
            if (x_pos>=0&&y_pos>=0)
              Pixel_background(x_pos,y_pos,real_color);
      }
    }
  }
  Update_rect(MENU_WIDTH*Menu_factor_X,Menu_Y_before_window,Screen_width-(MENU_WIDTH*Menu_factor_X),(Menu_height-11)*Menu_factor_Y);
}

// -------- Calcul des bornes de la partie d'image visible � l'�cran ---------
void Compute_limits(void)
/*
  Avant l'appel � cette fonction, les donn�es de la loupe doivent �tre � jour.
*/
{
  if (Main_magnifier_mode)
  {
    // -- Calcul des limites de la partie non zoom�e de l'image --
    Limit_top  =Main_offset_Y;
    Limit_left=Main_offset_X;
    Limit_visible_bottom   =Limit_top+Menu_Y-1;
    Limit_visible_right=Limit_left+Main_separator_position-1;

    if (Limit_visible_bottom>=Main_image_height)
      Limit_bottom=Main_image_height-1;
    else
      Limit_bottom=Limit_visible_bottom;

    if (Limit_visible_right>=Main_image_width)
      Limit_right=Main_image_width-1;
    else
      Limit_right=Limit_visible_right;

    // -- Calcul des limites de la partie zoom�e de l'image --
    Limit_top_zoom  =Main_magnifier_offset_Y;
    Limit_left_zoom=Main_magnifier_offset_X;
    Limit_visible_bottom_zoom   =Limit_top_zoom+Main_magnifier_height-1;
    Limit_visible_right_zoom=Limit_left_zoom+Main_magnifier_width-1;

    if (Limit_visible_bottom_zoom>=Main_image_height)
      Limit_bottom_zoom=Main_image_height-1;
    else
      Limit_bottom_zoom=Limit_visible_bottom_zoom;

    if (Limit_visible_right_zoom>=Main_image_width)
      Limit_right_zoom=Main_image_width-1;
    else
      Limit_right_zoom=Limit_visible_right_zoom;
  }
  else
  {
    // -- Calcul des limites de la partie visible de l'image --
    Limit_top  =Main_offset_Y;
    Limit_left=Main_offset_X;
    Limit_visible_bottom   =Limit_top+(Menu_is_visible?Menu_Y:Screen_height)-1; // A REVOIR POUR SIMPLIFICATION
    Limit_visible_right=Limit_left+Screen_width-1;

    if (Limit_visible_bottom>=Main_image_height)
      Limit_bottom=Main_image_height-1;
    else
      Limit_bottom=Limit_visible_bottom;

    if (Limit_visible_right>=Main_image_width)
      Limit_right=Main_image_width-1;
    else
      Limit_right=Limit_visible_right;
  }
}


// -- Calculer les coordonn�es du pinceau en fonction du snap et de la loupe -
void Compute_paintbrush_coordinates(void)
{
  if ((Main_magnifier_mode) && (Mouse_X>=Main_X_zoom))
  {
    Paintbrush_X=((Mouse_X-Main_X_zoom)/Main_magnifier_factor)+Main_magnifier_offset_X;
    Paintbrush_Y=(Mouse_Y/Main_magnifier_factor)+Main_magnifier_offset_Y;
  }
  else
  {
    Paintbrush_X=Mouse_X+Main_offset_X;
    Paintbrush_Y=Mouse_Y+Main_offset_Y;
  }

  if (Snap_mode)
  {
    Paintbrush_X=(((Paintbrush_X+(Snap_width>>1)-Snap_offset_X)/Snap_width)*Snap_width)+Snap_offset_X;
    Paintbrush_Y=(((Paintbrush_Y+(Snap_height>>1)-Snap_offset_Y)/Snap_height)*Snap_height)+Snap_offset_Y;
  }
  
  // Handling the snap axis mode, when shift is pressed.
  switch (Current_operation)
  {
    // Operations that don't implement it
    case OPERATION_LINE:
	case OPERATION_ROTATE_BRUSH:
      Snap_axis=0;
      break;
    // Operations that implement it
    default:
      if (Snap_axis==0 && (SDL_GetModState() & KMOD_SHIFT))
      {
        // Start "Snap axis" mode
        Snap_axis=1;
        Snap_axis_origin_X=Paintbrush_X;
        Snap_axis_origin_Y=Paintbrush_Y;
      } 
  }

  if (Snap_axis==1)
  {
    // Cursor moved
    if (Paintbrush_X != Snap_axis_origin_X || Paintbrush_Y != Snap_axis_origin_Y)
    {
      if ((Paintbrush_X-Snap_axis_origin_X)*(Paintbrush_X-Snap_axis_origin_X) >
          (Paintbrush_Y-Snap_axis_origin_Y)*(Paintbrush_Y-Snap_axis_origin_Y))
      // Displacement was bigger on X axis: lock Y
        Snap_axis=2;
      else
        Snap_axis=3;
    }
  }
  if (Snap_axis==2)
  {
    Paintbrush_Y = Snap_axis_origin_Y;
  }
  else if (Snap_axis==3)
  {
    Paintbrush_X = Snap_axis_origin_X;
  }
}



// -- Affichage de la limite de l'image -------------------------------------
void Display_image_limits(void)
{
  short start;
  short pos;
  short end;
  byte right_is_visible;
  byte bottom_is_visible;
  short old_zoom_limit;

  right_is_visible=Main_image_width<((Main_magnifier_mode)?Main_separator_position:Screen_width);
  bottom_is_visible   =Main_image_height<Menu_Y;


  // On v�rifie que la limite � droite est visible:
  if (right_is_visible)
  {
    start=Limit_top;
    end=(Limit_bottom<Main_image_height)?
        Limit_bottom:Main_image_height;

    if (bottom_is_visible)
      end++;

    // Juste le temps d'afficher les limites, on �tend les limites de la loupe
    // aux limites visibles, car sinon Pixel_preview ne voudra pas afficher.
    old_zoom_limit=Limit_right_zoom;
    Limit_right_zoom=Limit_visible_right_zoom;

    for (pos=start;pos<=end;pos++)
      Pixel_preview(Main_image_width,pos,((pos+Main_image_height)&1)?MC_White:MC_Black);

    Update_rect(Main_image_width,start,1,end-start + 1);
    // On restaure la bonne valeur des limites
    Limit_right_zoom=old_zoom_limit;
  }

  // On v�rifie que la limite en bas est visible:
  if (bottom_is_visible)
  {
    start=Limit_left;
    end=(Limit_right<Main_image_width)?
        Limit_right:Main_image_width;

    // On �tend �galement les limites en bas (comme pour la limite droit)
    old_zoom_limit=Limit_bottom_zoom;
    Limit_bottom_zoom=Limit_visible_bottom_zoom;

    for (pos=start;pos<=end;pos++)
      Pixel_preview(pos,Main_image_height,((pos+Main_image_height)&1)?MC_White:MC_Black);

    Update_rect(start,Main_image_height,end-start + 1,1);

    // On restaure la bonne valeur des limites
    Limit_bottom_zoom=old_zoom_limit;
  }
}



// -- Recadrer la partie non-zoom�e de l'image par rapport � la partie zoom�e
//    lorsqu'on scrolle en mode Loupe --
void Position_screen_according_to_zoom(void)
{
  // Centrage en X
  if (Main_image_width>Main_separator_position)
  {
    Main_offset_X=Main_magnifier_offset_X+(Main_magnifier_width>>1)
                         -(Main_separator_position>>1);
    if (Main_offset_X<0)
      Main_offset_X=0;
    else if (Main_image_width<Main_offset_X+Main_separator_position)
      Main_offset_X=Main_image_width-Main_separator_position;
  }
  else
    Main_offset_X=0;

  // Centrage en Y
  if (Main_image_height>Menu_Y)
  {
    Main_offset_Y=Main_magnifier_offset_Y+(Main_magnifier_height>>1)
                         -(Menu_Y>>1);
    if (Main_offset_Y<0)
      Main_offset_Y=0;
    else if (Main_image_height<Main_offset_Y+Menu_Y)
      Main_offset_Y=Main_image_height-Menu_Y;
  }
  else
    Main_offset_Y=0;
}

// -- Recenter the non-zoomed part of image around a precise pixel
void Position_screen_according_to_position(int target_x, int target_y)
{
  // Centrage en X
  if (Main_image_width>Main_separator_position)
  {
    Main_offset_X=target_x-Mouse_X;
    // Do not allow the zoomed part to show something that the
    // non-zoomed part doesn't see. All clipping is computed according
    // to the non-zoomed part.
    if (Main_magnifier_offset_X<Main_offset_X)
      Main_offset_X=Main_magnifier_offset_X;
    else if (Main_magnifier_offset_X+Main_magnifier_width > Main_offset_X+Main_separator_position)
      Main_offset_X = Main_magnifier_offset_X+Main_magnifier_width-Main_separator_position;
    if (Main_offset_X<0)
      Main_offset_X=0;
    else if (Main_image_width<Main_offset_X+Main_separator_position)
      Main_offset_X=Main_image_width-Main_separator_position;
      
    
  }
  else
    Main_offset_X=0;

  // Centrage en Y
  if (Main_image_height>Menu_Y)
  {
    Main_offset_Y=target_y-Mouse_Y;
    // Do not allow the zoomed part to show something that the
    // non-zoomed part doesn't see. All clipping is computed according
    // to the non-zoomed part.
    if (Main_magnifier_offset_Y<Main_offset_Y)
      Main_offset_Y=Main_magnifier_offset_Y;
    else if (Main_magnifier_offset_Y+Main_magnifier_height > Main_offset_Y)
      Main_offset_Y = Main_magnifier_offset_Y+Main_magnifier_height;
    if (Main_offset_Y<0)
      Main_offset_Y=0;
    else if (Main_image_height<Main_offset_Y+Menu_Y)
      Main_offset_Y=Main_image_height-Menu_Y;
  }
  else
    Main_offset_Y=0;
}


// - Calcul des donn�es du split en fonction de la proportion de chaque zone -
void Compute_separator_data(void)
{
  //short temp;
  short theoric_X=Round(Main_separator_proportion*Screen_width);

  Main_X_zoom=Screen_width-(((Screen_width+(Main_magnifier_factor>>1)-theoric_X)/Main_magnifier_factor)*Main_magnifier_factor);
  Main_separator_position=Main_X_zoom-(Menu_factor_X*SEPARATOR_WIDTH);

  // Correction en cas de d�bordement sur la gauche
  while (Main_separator_position*(Main_magnifier_factor+1)<Screen_width-(Menu_factor_X*SEPARATOR_WIDTH))
  {
    Main_separator_position+=Main_magnifier_factor;
    Main_X_zoom+=Main_magnifier_factor;
  }
  // Correction en cas de d�bordement sur la droite
  theoric_X=Screen_width-((NB_ZOOMED_PIXELS_MIN-1)*Main_magnifier_factor);
  while (Main_X_zoom>=theoric_X)
  {
    Main_separator_position-=Main_magnifier_factor;
    Main_X_zoom-=Main_magnifier_factor;
  }
}



// -------------------- Calcul des information de la loupe -------------------
void Compute_magnifier_data(void)
/*
  Apr�s modification des donn�es de la loupe, il faut recalculer les limites.
*/
{
  Compute_separator_data();

  Main_magnifier_width=(Screen_width-Main_X_zoom)/Main_magnifier_factor;

  Main_magnifier_height=Menu_Y/Main_magnifier_factor;
  if (Menu_Y%Main_magnifier_factor)
    Main_magnifier_height++;

  Clip_magnifier_offsets(&Main_magnifier_offset_X, &Main_magnifier_offset_Y);
}

void Clip_magnifier_offsets(short *x_offset, short *y_offset)
{
  if (Main_magnifier_mode)
  {
    if (*x_offset)
    {
      if (Main_image_width<*x_offset+Main_magnifier_width)
        *x_offset=Main_image_width-Main_magnifier_width;
      if (*x_offset<0)
        *x_offset=0;
    }
    if (*y_offset)
    {
      if (Main_image_height<*y_offset+Main_magnifier_height)
        *y_offset=Main_image_height-Main_magnifier_height+(Main_magnifier_height*Main_magnifier_factor-Menu_Y>=Main_magnifier_factor/2);
      if (*y_offset<0)
        *y_offset=0;
    }
  }
}

/// Changes magnifier factor and updates everything needed
void Change_magnifier_factor(byte factor_index, byte point_at_mouse)
{
  int target_x,target_y; // These coordinates are in image space
  byte magnified_view_leads=1;

  // Values that need to be computed before switching to the new zoom factor
  if (!point_at_mouse || Cursor_in_menu || !Main_magnifier_mode)
  {
    // Locate the pixel in center of the magnified area
    target_x = Main_magnifier_offset_X + (Main_magnifier_width >> 1);
    target_y = Main_magnifier_offset_Y + (Main_magnifier_height >> 1);
    point_at_mouse=0;
  }
  else if (Mouse_X>=Main_X_zoom)
  {
    // Locate the pixel under the cursor, in magnified area
    target_x=((Mouse_X-Main_X_zoom)/Main_magnifier_factor)+Main_magnifier_offset_X;
    target_y=(Mouse_Y/Main_magnifier_factor)+Main_magnifier_offset_Y;
    point_at_mouse=1;
  }
  else
  {
    // Locate the pixel under the cursor, in normal area
    target_x=Mouse_X+Main_offset_X;
    target_y=Mouse_Y+Main_offset_Y;
    magnified_view_leads=0;
    point_at_mouse=0;
  }

  Main_magnifier_factor=ZOOM_FACTOR[factor_index];
  Compute_magnifier_data();

  if (Main_magnifier_mode)
  {
    // Recompute the magnifier offset (center its view)
    if (point_at_mouse)
    {
      // Target pixel must be located under the mouse position.
      Main_magnifier_offset_X = target_x-((Mouse_X-Main_X_zoom)/Main_magnifier_factor);
      Main_magnifier_offset_Y = target_y-((Mouse_Y)/Main_magnifier_factor);
    }
    else
    {
      // Target pixel must be positioned at new center
      Main_magnifier_offset_X = target_x-(Main_magnifier_width>>1);
      Main_magnifier_offset_Y = target_y-(Main_magnifier_height>>1);
    }
    // Fix cases where the image would overflow on edges
    Clip_magnifier_offsets(&Main_magnifier_offset_X, &Main_magnifier_offset_Y);

    if (magnified_view_leads)
      Position_screen_according_to_zoom();
    else
      Position_screen_according_to_position(target_x, target_y);

    Pixel_preview=Pixel_preview_magnifier;

  }
  else
    Pixel_preview=Pixel_preview_normal;

  Compute_limits();
  Compute_paintbrush_coordinates();
}

void Copy_view_to_spare(void)
{
  
  // Don't do anything if the pictures have different dimensions
  if (Main_image_width!=Spare_image_width || Main_image_height!=Spare_image_height)
    return;
  
  // Copie des d�calages de la fen�tre principale (non zoom�e) de l'image
  Spare_offset_X=Main_offset_X;
  Spare_offset_Y=Main_offset_Y;

  // Copie du bool�en "Mode loupe" de l'image
  Spare_magnifier_mode=Main_magnifier_mode;

  // Copie du facteur de zoom du brouillon
  Spare_magnifier_factor=Main_magnifier_factor;

  // Copie des dimensions de la fen�tre de zoom
  Spare_magnifier_width=Main_magnifier_width;
  Spare_magnifier_height=Main_magnifier_height;

  // Copie des d�calages de la fen�tre de zoom
  Spare_magnifier_offset_X=Main_magnifier_offset_X;
  Spare_magnifier_offset_Y=Main_magnifier_offset_Y;

  // Copie des donn�es du split du zoom
  Spare_separator_position=Main_separator_position;
  Spare_X_zoom=Main_X_zoom;
  Spare_separator_proportion=Main_separator_proportion;
}

  // -- Afficher la barre de s�paration entre les parties zoom�es ou non en
  //    mode Loupe --

void Display_separator(void)
{
  // Partie grise du milieu
  Block(Main_separator_position+(Menu_factor_X<<1),Menu_factor_Y,
        (SEPARATOR_WIDTH-4)*Menu_factor_X,
        Menu_Y-(Menu_factor_Y<<1),MC_Light);

  // Barre noire de gauche
  Block(Main_separator_position,0,Menu_factor_X,Menu_Y,MC_Black);

  // Barre noire de droite
  Block(Main_X_zoom-Menu_factor_X,0,Menu_factor_X,Menu_Y,MC_Black);

  // Bord haut (blanc)
  Block(Main_separator_position+Menu_factor_X,0,
        (SEPARATOR_WIDTH-3)*Menu_factor_X,Menu_factor_Y,MC_White);

  // Bord gauche (blanc)
  Block(Main_separator_position+Menu_factor_X,Menu_factor_Y,
        Menu_factor_X,(Menu_Y-(Menu_factor_Y<<1)),MC_White);

  // Bord droite (gris fonc�)
  Block(Main_X_zoom-(Menu_factor_X<<1),Menu_factor_Y,
        Menu_factor_X,(Menu_Y-(Menu_factor_Y<<1)),MC_Dark);

  // Bord bas (gris fonc�)
  Block(Main_separator_position+(Menu_factor_X<<1),Menu_Y-Menu_factor_Y,
        (SEPARATOR_WIDTH-3)*Menu_factor_X,Menu_factor_Y,MC_Dark);

  // Coin bas gauche
  Block(Main_separator_position+Menu_factor_X,Menu_Y-Menu_factor_Y,
        Menu_factor_X,Menu_factor_Y,MC_Light);
  // Coin haut droite
  Block(Main_X_zoom-(Menu_factor_X<<1),0,
        Menu_factor_X,Menu_factor_Y,MC_Light);

  Update_rect(Main_separator_position,0,SEPARATOR_WIDTH*Menu_factor_X,Menu_Y); // On r�affiche toute la partie � gauche du split, ce qui permet d'effacer son ancienne position
}



// -- Fonctions de manipulation du curseur -----------------------------------


  // -- Afficher une barre horizontale XOR zoom�e

void Horizontal_XOR_line_zoom(short x_pos, short y_pos, short width)
{
  short real_x_pos=Main_X_zoom+(x_pos-Main_magnifier_offset_X)*Main_magnifier_factor;
  short real_y_pos=(y_pos-Main_magnifier_offset_Y)*Main_magnifier_factor;
  short real_width=width*Main_magnifier_factor;
  short end_y_pos=(real_y_pos+Main_magnifier_factor<Menu_Y)?real_y_pos+Main_magnifier_factor:Menu_Y;
  short index;

  for (index=real_y_pos; index<end_y_pos; index++)
    Horizontal_XOR_line(real_x_pos,index,real_width);

  Update_rect(real_x_pos,real_y_pos,real_width,end_y_pos-real_y_pos);
}


  // -- Afficher une barre verticale XOR zoom�e

void Vertical_XOR_line_zoom(short x_pos, short y_pos, short height)
{
  short real_x_pos=Main_X_zoom+(x_pos-Main_magnifier_offset_X)*Main_magnifier_factor;
  short real_y_pos=(y_pos-Main_magnifier_offset_Y)*Main_magnifier_factor;
  short end_y_pos=(real_y_pos+height*Main_magnifier_factor<Menu_Y)?real_y_pos+(height*Main_magnifier_factor):Menu_Y;
  short index;

  for (index=real_y_pos; index<end_y_pos; index++)
    Horizontal_XOR_line(real_x_pos,index,Main_magnifier_factor);

  Update_rect(real_x_pos,real_y_pos,Main_magnifier_factor,end_y_pos-real_y_pos);
}


  // -- Afficher le curseur --

void Display_cursor(void)
{
  byte  shape;
  short start_x;
  short start_y;
  short end_x;
  short end_y;
  short x_pos;
  short y_pos;
  short counter_x = 0;
  short counter_y;
  int   temp;
  byte  color;
  float cos_a,sin_a;
  short x1,y1,x2,y2,x3,y3,x4,y4;

  // Si le curseur est dans le menu ou sur la barre de split, on affiche toujours une fl�che.
  if ( ( (Mouse_Y<Menu_Y)
      && ( (!Main_magnifier_mode) || (Mouse_X<Main_separator_position) || (Mouse_X>=Main_X_zoom) ) )
    || (Windows_open) || (Cursor_shape==CURSOR_SHAPE_HOURGLASS) )
    shape=Cursor_shape;
  else
    shape=CURSOR_SHAPE_ARROW;

  switch(shape)
  {
    case CURSOR_SHAPE_TARGET :
      if (!Paintbrush_hidden)
        Display_paintbrush(Paintbrush_X,Paintbrush_Y,Fore_color);
      if (!Cursor_hidden)
      {
        if (Config.Cursor==1)
        {
          start_y=(Mouse_Y<6)?6-Mouse_Y:0;
          if (start_y<4)
            Vertical_XOR_line  (Mouse_X,Mouse_Y+start_y-6,4-start_y);

          start_x=(Mouse_X<6)?(short)6-Mouse_X:0;
          if (start_x<4)
            Horizontal_XOR_line(Mouse_X+start_x-6,Mouse_Y,4-start_x);

          end_x=(Mouse_X+7>Screen_width)?Mouse_X+7-Screen_width:0;
          if (end_x<4)
            Horizontal_XOR_line(Mouse_X+3,Mouse_Y,4-end_x);

          end_y=(Mouse_Y+7>Screen_height)?Mouse_Y+7-Screen_height:0;
          if (end_y<4)
            Vertical_XOR_line  (Mouse_X,Mouse_Y+3,4-end_y);

          Update_rect(Mouse_X+start_x-6,Mouse_Y+start_y-6,13-end_x,13-end_y);
        }
        else
        {
          temp=(Config.Cursor)?CURSOR_SHAPE_THIN_TARGET:CURSOR_SHAPE_TARGET;
          start_x=Mouse_X-Gfx->Cursor_offset_X[temp];
          start_y=Mouse_Y-Gfx->Cursor_offset_Y[temp];

          for (y_pos=start_y,counter_y=0; counter_y<15 && y_pos < Screen_height;
            y_pos++,counter_y++)
          {
            if( y_pos < 0 ) continue;
              for (x_pos=start_x,counter_x=0;
                counter_x<15 && x_pos < Screen_width; x_pos++,counter_x++)
              {
                if( x_pos < 0 ) continue;
                  color=Gfx->Cursor_sprite[temp][counter_y][counter_x];
                  Cursor_background[counter_y][counter_x]=Read_pixel(x_pos,y_pos);
                  if (color!=MC_Trans)
                    Pixel(x_pos,y_pos,color);
            }
          }

          Update_rect(Max(start_x,0),Max(start_y,0),counter_x,counter_y);
        }
      }
      break;

    case CURSOR_SHAPE_COLORPICKER:
      if (!Paintbrush_hidden)
        Display_paintbrush(Paintbrush_X,Paintbrush_Y,Fore_color);
      if (Config.Cursor==1)
      {
        // Barres formant la croix principale

        start_y=(Mouse_Y<5)?5-Mouse_Y:0;
        if (start_y<3)
          Vertical_XOR_line  (Mouse_X,Mouse_Y+start_y-5,3-start_y);

        start_x=(Mouse_X<5)?(short)5-Mouse_X:0;
        if (start_x<3)
          Horizontal_XOR_line(Mouse_X+start_x-5,Mouse_Y,3-start_x);

        end_x=(Mouse_X+6>Screen_width)?Mouse_X+6-Screen_width:0;
        if (end_x<3)
          Horizontal_XOR_line(Mouse_X+3,Mouse_Y,3-end_x);

        end_y=(Mouse_Y+6>Menu_Y/*Screen_height*/)?Mouse_Y+6-Menu_Y/*Screen_height*/:0;
        if (end_y<3)
          Vertical_XOR_line  (Mouse_X,Mouse_Y+3,3-end_y);

        // Petites barres aux extr�mit�s

        start_x=(!Mouse_X);
        start_y=(!Mouse_Y);
        end_x=(Mouse_X>=Screen_width-1);
        end_y=(Mouse_Y>=Menu_Y-1);

        if (Mouse_Y>5)
          Horizontal_XOR_line(start_x+Mouse_X-1,Mouse_Y-6,3-(start_x+end_x));

        if (Mouse_X>5)
          Vertical_XOR_line  (Mouse_X-6,start_y+Mouse_Y-1,3-(start_y+end_y));

        if (Mouse_X<Screen_width-6)
          Vertical_XOR_line  (Mouse_X+6,start_y+Mouse_Y-1,3-(start_y+end_y));

        if (Mouse_Y<Menu_Y-6)
          Horizontal_XOR_line(start_x+Mouse_X-1,Mouse_Y+6,3-(start_x+end_x));
      }
      else
      {
        temp=(Config.Cursor)?CURSOR_SHAPE_THIN_COLORPICKER:CURSOR_SHAPE_COLORPICKER;
        start_x=Mouse_X-Gfx->Cursor_offset_X[temp];
        start_y=Mouse_Y-Gfx->Cursor_offset_Y[temp];

        for (y_pos=start_y,counter_y=0;counter_y<15;y_pos++,counter_y++)
        {
          if(y_pos<0) continue;
          if(y_pos>=Screen_height) break;
          for (x_pos=start_x,counter_x=0;counter_x<15;x_pos++,counter_x++)
          {
            if(x_pos<0) continue;
            if(x_pos>=Screen_width) break;
            color=Gfx->Cursor_sprite[temp][counter_y][counter_x];
            // On sauvegarde dans Cursor_background pour restaurer plus tard
            Cursor_background[counter_y][counter_x]=Read_pixel(x_pos,y_pos);
            if (color!=MC_Trans)
              Pixel(x_pos,y_pos,color);
          }
        }
        Update_rect(Max(start_x,0),Max(start_y,0),counter_x,counter_y);
      }
      break;

    case CURSOR_SHAPE_MULTIDIRECTIONAL :
    case CURSOR_SHAPE_HORIZONTAL :
    case CURSOR_SHAPE_BUCKET :
      if (Cursor_hidden)
        break;

    case CURSOR_SHAPE_ARROW :
    case CURSOR_SHAPE_HOURGLASS :
      start_x=Mouse_X-Gfx->Cursor_offset_X[shape];
      start_y=Mouse_Y-Gfx->Cursor_offset_Y[shape];
      for (y_pos=start_y,counter_y=0;counter_y<CURSOR_SPRITE_HEIGHT;y_pos++,counter_y++)
      {
        if(y_pos<0) continue;
        if(y_pos>=Screen_height) break;
        for (x_pos=start_x,counter_x=0;counter_x<CURSOR_SPRITE_WIDTH;x_pos++,counter_x++)
        {
          if(x_pos<0) continue;
          if(x_pos>=Screen_width) break;
          color=Gfx->Cursor_sprite[shape][counter_y][counter_x];
          // On sauvegarde dans Cursor_background pour restaurer plus tard
          Cursor_background[counter_y][counter_x]=Read_pixel(x_pos,y_pos);
          if (color!=MC_Trans)
            Pixel(x_pos,y_pos,color);
        }
      }
      Update_rect(Max(start_x,0),Max(start_y,0),counter_x,counter_y);
      break;

    case CURSOR_SHAPE_XOR_TARGET :
      x_pos=Paintbrush_X-Main_offset_X;
      y_pos=Paintbrush_Y-Main_offset_Y;

      counter_x=(Main_magnifier_mode)?Main_separator_position:Screen_width; // width de la barre XOR
      if ((y_pos<Menu_Y) && (Paintbrush_Y>=Limit_top))
      {
        Horizontal_XOR_line(0,Paintbrush_Y-Main_offset_Y,counter_x);
        Update_rect(0,Paintbrush_Y-Main_offset_Y,counter_x,1);
      }

      if ((x_pos<counter_x) && (Paintbrush_X>=Limit_left))
      {
        Vertical_XOR_line(Paintbrush_X-Main_offset_X,0,Menu_Y);
        Update_rect(Paintbrush_X-Main_offset_X,0,1,Menu_Y);
      }

      if (Main_magnifier_mode)
      {
        // UPDATERECT
        if ((Paintbrush_Y>=Limit_top_zoom) && (Paintbrush_Y<=Limit_visible_bottom_zoom))
          Horizontal_XOR_line_zoom(Limit_left_zoom,Paintbrush_Y,Main_magnifier_width);
        if ((Paintbrush_X>=Limit_left_zoom) && (Paintbrush_X<=Limit_visible_right_zoom))
          Vertical_XOR_line_zoom(Paintbrush_X,Limit_top_zoom,Main_magnifier_height);
      }
      break;
    case CURSOR_SHAPE_XOR_RECTANGLE :
      // !!! Cette forme ne peut pas �tre utilis�e en mode Loupe !!!

      // Petite croix au centre
      start_x=(Mouse_X-3);
      start_y=(Mouse_Y-3);
      end_x  =(Mouse_X+4);
      end_y  =(Mouse_Y+4);
      if (start_x<0)
        start_x=0;
      if (start_y<0)
        start_y=0;
      if (end_x>Screen_width)
        end_x=Screen_width;
      if (end_y>Menu_Y)
        end_y=Menu_Y;

      Horizontal_XOR_line(start_x,Mouse_Y,end_x-start_x);
      Vertical_XOR_line  (Mouse_X,start_y,end_y-start_y);

      // Grand rectangle autour
      start_x=Mouse_X-(Main_magnifier_width>>1);
      start_y=Mouse_Y-(Main_magnifier_height>>1);
      if (start_x+Main_magnifier_width>=Limit_right-Main_offset_X)
        start_x=Limit_right-Main_magnifier_width-Main_offset_X+1;
      if (start_y+Main_magnifier_height>=Limit_bottom-Main_offset_Y)
        start_y=Limit_bottom-Main_magnifier_height-Main_offset_Y+1;
      if (start_x<0)
        start_x=0;
      if (start_y<0)
        start_y=0;
      end_x=start_x+Main_magnifier_width-1;
      end_y=start_y+Main_magnifier_height-1;

      Horizontal_XOR_line(start_x,start_y,Main_magnifier_width);
      Vertical_XOR_line(start_x,start_y+1,Main_magnifier_height-2);
      Vertical_XOR_line(  end_x,start_y+1,Main_magnifier_height-2);
      Horizontal_XOR_line(start_x,  end_y,Main_magnifier_width);

      Update_rect(start_x,start_y,end_x+1-start_x,end_y+1-start_y);

      break;
    default: //case CURSOR_SHAPE_XOR_ROTATION :
      start_x=1-(Brush_width>>1);
      start_y=1-(Brush_height>>1);
      end_x=start_x+Brush_width-1;
      end_y=start_y+Brush_height-1;

      if (Brush_rotation_center_is_defined)
      {
        if ( (Brush_rotation_center_X==Paintbrush_X)
          && (Brush_rotation_center_Y==Paintbrush_Y) )
        {
          cos_a=1.0;
          sin_a=0.0;
        }
        else
        {
          x_pos=Paintbrush_X-Brush_rotation_center_X;
          y_pos=Paintbrush_Y-Brush_rotation_center_Y;
          cos_a=(float)x_pos/sqrt((x_pos*x_pos)+(y_pos*y_pos));
          sin_a=sin(acos(cos_a));
          if (y_pos>0) sin_a=-sin_a;
        }

        Transform_point(start_x,start_y, cos_a,sin_a, &x1,&y1);
        Transform_point(end_x  ,start_y, cos_a,sin_a, &x2,&y2);
        Transform_point(start_x,end_y  , cos_a,sin_a, &x3,&y3);
        Transform_point(end_x  ,end_y  , cos_a,sin_a, &x4,&y4);

        x1+=Brush_rotation_center_X;
        y1+=Brush_rotation_center_Y;
        x2+=Brush_rotation_center_X;
        y2+=Brush_rotation_center_Y;
        x3+=Brush_rotation_center_X;
        y3+=Brush_rotation_center_Y;
        x4+=Brush_rotation_center_X;
        y4+=Brush_rotation_center_Y;
        Pixel_figure_preview_xor(Brush_rotation_center_X,Brush_rotation_center_Y,0);
        Draw_line_preview_xor(Brush_rotation_center_X,Brush_rotation_center_Y,Paintbrush_X,Paintbrush_Y,0);
      }
      else
      {
        x1=x3=1-Brush_width;
        y1=y2=start_y;
        x2=x4=Paintbrush_X;
        y3=y4=end_y;

        x1+=Paintbrush_X;
        y1+=Paintbrush_Y;
        y2+=Paintbrush_Y;
        x3+=Paintbrush_X;
        y3+=Paintbrush_Y;
        y4+=Paintbrush_Y;
        Pixel_figure_preview_xor(Paintbrush_X-end_x,Paintbrush_Y,0);
        Draw_line_preview_xor(Paintbrush_X-end_x,Paintbrush_Y,Paintbrush_X,Paintbrush_Y,0);
      }

      Draw_line_preview_xor(x1,y1,x2,y2,0);
      Draw_line_preview_xor(x2,y2,x4,y4,0);
      Draw_line_preview_xor(x4,y4,x3,y3,0);
      Draw_line_preview_xor(x3,y3,x1,y1,0);
  }
}

  // -- Effacer le curseur --

void Hide_cursor(void)
{
  byte  shape;
  int start_x; // int car sont parfois n�gatifs ! (quand on dessine sur un bord)
  int start_y;
  short end_x;
  short end_y;
  int x_pos = 0;
  int y_pos;
  short counter_x = 0;
  short counter_y;
  int   temp;
  float cos_a,sin_a;
  short x1,y1,x2,y2,x3,y3,x4,y4;

  if ( ( (Mouse_Y<Menu_Y)
      && ( (!Main_magnifier_mode) || (Mouse_X<Main_separator_position)
                         || (Mouse_X>=Main_X_zoom) ) )
    || (Windows_open) || (Cursor_shape==CURSOR_SHAPE_HOURGLASS) )
    shape=Cursor_shape;
  else
    shape=CURSOR_SHAPE_ARROW;

  switch(shape)
  {
    case CURSOR_SHAPE_TARGET :
      if (!Cursor_hidden)
      {
        if (Config.Cursor==1)
        {
          start_y=(Mouse_Y<6)?6-Mouse_Y:0;
          if (start_y<4)
            Vertical_XOR_line  (Mouse_X,Mouse_Y+start_y-6,4-start_y);

          start_x=(Mouse_X<6)?(short)6-Mouse_X:0;
          if (start_x<4)
            Horizontal_XOR_line(Mouse_X+start_x-6,Mouse_Y,4-start_x);

          end_x=(Mouse_X+7>Screen_width)?Mouse_X+7-Screen_width:0;
          if (end_x<4)
            Horizontal_XOR_line(Mouse_X+3,Mouse_Y,4-end_x);

          end_y=(Mouse_Y+7>Screen_height)?Mouse_Y+7-Screen_height:0;
          if (end_y<4)
            Vertical_XOR_line  (Mouse_X,Mouse_Y+3,4-end_y);

          Update_rect(Mouse_X+start_x-6,Mouse_Y+start_y-6,13-end_x,13-end_y);
        }
        else
        {
          temp=(Config.Cursor)?CURSOR_SHAPE_THIN_TARGET:CURSOR_SHAPE_TARGET;
          start_x=Mouse_X-Gfx->Cursor_offset_X[temp];
          start_y=Mouse_Y-Gfx->Cursor_offset_Y[temp];

          for (y_pos=start_y,counter_y=0;counter_y<15;y_pos++,counter_y++)
          {
            if(y_pos < 0) continue;
            if(y_pos>=Screen_height) break;
            for (x_pos=start_x,counter_x=0;counter_x<15;x_pos++,counter_x++)
            {
              if(x_pos < 0) continue;
              else if (x_pos>=Screen_width) break;
              Pixel(x_pos,y_pos,Cursor_background[counter_y][counter_x]);
            }
          }

          Update_rect(Max(start_x,0),Max(start_y,0),x_pos-start_x,y_pos-start_y);
        }
      }
      if (!Paintbrush_hidden)
      {
        Hide_paintbrush(Paintbrush_X,Paintbrush_Y);
      }
      break;

    case CURSOR_SHAPE_COLORPICKER:
        if (Config.Cursor==1)
        {
          // Barres formant la croix principale

          start_y=(Mouse_Y<5)?5-Mouse_Y:0;
          if (start_y<3)
            Vertical_XOR_line  (Mouse_X,Mouse_Y+start_y-5,3-start_y);

          start_x=(Mouse_X<5)?(short)5-Mouse_X:0;
          if (start_x<3)
            Horizontal_XOR_line(Mouse_X+start_x-5,Mouse_Y,3-start_x);

          end_x=(Mouse_X+6>Screen_width)?Mouse_X+6-Screen_width:0;
          if (end_x<3)
            Horizontal_XOR_line(Mouse_X+3,Mouse_Y,3-end_x);

          end_y=(Mouse_Y+6>Screen_height)?Mouse_Y+6-Screen_height:0;
          if (end_y<3)
            Vertical_XOR_line  (Mouse_X,Mouse_Y+3,3-end_y);

          start_x=(!Mouse_X);
          start_y=(!Mouse_Y);
          end_x=(Mouse_X>=Screen_width-1);
          end_y=(Mouse_Y>=Menu_Y-1);

          if (Mouse_Y>5)
            Horizontal_XOR_line(start_x+Mouse_X-1,Mouse_Y-6,3-(start_x+end_x));

          if (Mouse_X>5)
            Vertical_XOR_line  (Mouse_X-6,start_y+Mouse_Y-1,3-(start_y+end_y));

          if (Mouse_X<Screen_width-6)
            Vertical_XOR_line  (Mouse_X+6,start_y+Mouse_Y-1,3-(start_y+end_y));

          if (Mouse_Y<Menu_Y-6)
            Horizontal_XOR_line(start_x+Mouse_X-1,Mouse_Y+6,3-(start_x+end_x));

          Update_rect(start_x,start_y,end_x-start_x,end_y-start_y);
        }
        else
        {
          temp=(Config.Cursor)?CURSOR_SHAPE_THIN_COLORPICKER:CURSOR_SHAPE_COLORPICKER;
          start_x=Mouse_X-Gfx->Cursor_offset_X[temp];
          start_y=Mouse_Y-Gfx->Cursor_offset_Y[temp];

          for (y_pos=start_y,counter_y=0;counter_y<15;y_pos++,counter_y++)
          {
            if(y_pos<0) continue;
            if(y_pos>=Screen_height) break;
            for (x_pos=start_x,counter_x=0;counter_x<15;x_pos++,counter_x++)
            {
              if(x_pos<0) continue;
              if(x_pos>=Screen_width) break;
                  Pixel(x_pos,y_pos,Cursor_background[counter_y][counter_x]);
            }
          }
          Update_rect(Max(start_x,0),Max(start_y,0),counter_x,counter_y);
        }
      if (!Paintbrush_hidden)
        Hide_paintbrush(Paintbrush_X,Paintbrush_Y);
      break;

    case CURSOR_SHAPE_MULTIDIRECTIONAL :
    case CURSOR_SHAPE_HORIZONTAL :
    case CURSOR_SHAPE_BUCKET :
      if (Cursor_hidden)
        break;

    case CURSOR_SHAPE_ARROW :
    case CURSOR_SHAPE_HOURGLASS :
      start_x=Mouse_X-Gfx->Cursor_offset_X[shape];
      start_y=Mouse_Y-Gfx->Cursor_offset_Y[shape];

      for (y_pos=start_y,counter_y=0;counter_y<CURSOR_SPRITE_HEIGHT;y_pos++,counter_y++)
      {
        if(y_pos<0) continue;
        if(y_pos>=Screen_height) break;
        for (x_pos=start_x,counter_x=0;counter_x<CURSOR_SPRITE_WIDTH;x_pos++,counter_x++)
        {
          if(x_pos<0) continue;
          if(x_pos>=Screen_width) break;
            Pixel(x_pos,y_pos,Cursor_background[counter_y][counter_x]);
        }
      }
      Update_rect(Max(start_x,0),Max(start_y,0),counter_x,counter_y);
      break;

    case CURSOR_SHAPE_XOR_TARGET :
      x_pos=Paintbrush_X-Main_offset_X;
      y_pos=Paintbrush_Y-Main_offset_Y;

      counter_x=(Main_magnifier_mode)?Main_separator_position:Screen_width; // width de la barre XOR
      if ((y_pos<Menu_Y) && (Paintbrush_Y>=Limit_top))
      {
        Horizontal_XOR_line(0,Paintbrush_Y-Main_offset_Y,counter_x);
        Update_rect(0,Paintbrush_Y-Main_offset_Y,counter_x,1);
      }

      if ((x_pos<counter_x) && (Paintbrush_X>=Limit_left))
      {
        Vertical_XOR_line(Paintbrush_X-Main_offset_X,0,Menu_Y);
        Update_rect(Paintbrush_X-Main_offset_X,0,1,Menu_Y);
      }

      if (Main_magnifier_mode)
      {
        // UPDATERECT
        if ((Paintbrush_Y>=Limit_top_zoom) && (Paintbrush_Y<=Limit_visible_bottom_zoom))
          Horizontal_XOR_line_zoom(Limit_left_zoom,Paintbrush_Y,Main_magnifier_width);
        if ((Paintbrush_X>=Limit_left_zoom) && (Paintbrush_X<=Limit_visible_right_zoom))
          Vertical_XOR_line_zoom(Paintbrush_X,Limit_top_zoom,Main_magnifier_height);
      }


      break;
    case CURSOR_SHAPE_XOR_RECTANGLE :
      // !!! Cette forme ne peut pas �tre utilis�e en mode Loupe !!!

      // Petite croix au centre
      start_x=(Mouse_X-3);
      start_y=(Mouse_Y-3);
      end_x  =(Mouse_X+4);
      end_y  =(Mouse_Y+4);
      if (start_x<0)
        start_x=0;
      if (start_y<0)
        start_y=0;
      if (end_x>Screen_width)
        end_x=Screen_width;
      if (end_y>Menu_Y)
        end_y=Menu_Y;

      Horizontal_XOR_line(start_x,Mouse_Y,end_x-start_x);
      Vertical_XOR_line  (Mouse_X,start_y,end_y-start_y);

      // Grand rectangle autour

      start_x=Mouse_X-(Main_magnifier_width>>1);
      start_y=Mouse_Y-(Main_magnifier_height>>1);
      if (start_x+Main_magnifier_width>=Limit_right-Main_offset_X)
        start_x=Limit_right-Main_magnifier_width-Main_offset_X+1;
      if (start_y+Main_magnifier_height>=Limit_bottom-Main_offset_Y)
        start_y=Limit_bottom-Main_magnifier_height-Main_offset_Y+1;
      if (start_x<0)
        start_x=0;
      if (start_y<0)
        start_y=0;
      end_x=start_x+Main_magnifier_width-1;
      end_y=start_y+Main_magnifier_height-1;

      Horizontal_XOR_line(start_x,start_y,Main_magnifier_width);
      Vertical_XOR_line(start_x,start_y+1,Main_magnifier_height-2);
      Vertical_XOR_line(  end_x,start_y+1,Main_magnifier_height-2);
      Horizontal_XOR_line(start_x,  end_y,Main_magnifier_width);

      Update_rect(start_x,start_y,end_x+1-start_x,end_y+1-start_y);

      break;
    default: //case CURSOR_SHAPE_XOR_ROTATION :
      start_x=1-(Brush_width>>1);
      start_y=1-(Brush_height>>1);
      end_x=start_x+Brush_width-1;
      end_y=start_y+Brush_height-1;

      if (Brush_rotation_center_is_defined)
      {
        if ( (Brush_rotation_center_X==Paintbrush_X)
          && (Brush_rotation_center_Y==Paintbrush_Y) )
        {
          cos_a=1.0;
          sin_a=0.0;
        }
        else
        {
          x_pos=Paintbrush_X-Brush_rotation_center_X;
          y_pos=Paintbrush_Y-Brush_rotation_center_Y;
          cos_a=(float)x_pos/sqrt((x_pos*x_pos)+(y_pos*y_pos));
          sin_a=sin(acos(cos_a));
          if (y_pos>0) sin_a=-sin_a;
        }

        Transform_point(start_x,start_y, cos_a,sin_a, &x1,&y1);
        Transform_point(end_x  ,start_y, cos_a,sin_a, &x2,&y2);
        Transform_point(start_x,end_y  , cos_a,sin_a, &x3,&y3);
        Transform_point(end_x  ,end_y  , cos_a,sin_a, &x4,&y4);

        x1+=Brush_rotation_center_X;
        y1+=Brush_rotation_center_Y;
        x2+=Brush_rotation_center_X;
        y2+=Brush_rotation_center_Y;
        x3+=Brush_rotation_center_X;
        y3+=Brush_rotation_center_Y;
        x4+=Brush_rotation_center_X;
        y4+=Brush_rotation_center_Y;
        Pixel_figure_preview_xor(Brush_rotation_center_X,Brush_rotation_center_Y,0);
        Draw_line_preview_xor(Brush_rotation_center_X,Brush_rotation_center_Y,Paintbrush_X,Paintbrush_Y,0);
      }
      else
      {
        x1=x3=1-Brush_width;
        y1=y2=start_y;
        x2=x4=Paintbrush_X;
        y3=y4=end_y;

        x1+=Paintbrush_X;
        y1+=Paintbrush_Y;
        y2+=Paintbrush_Y;
        x3+=Paintbrush_X;
        y3+=Paintbrush_Y;
        y4+=Paintbrush_Y;
        Pixel_figure_preview_xor(Paintbrush_X-end_x,Paintbrush_Y,0);
        Draw_line_preview_xor(Paintbrush_X-end_x,Paintbrush_Y,Paintbrush_X,Paintbrush_Y,0);
      }

      Draw_line_preview_xor(x1,y1,x2,y2,0);
      Draw_line_preview_xor(x2,y2,x4,y4,0);
      Draw_line_preview_xor(x4,y4,x3,y3,0);
      Draw_line_preview_xor(x3,y3,x1,y1,0);
  }
}



// -- Fonction diverses d'affichage ------------------------------------------

  // -- Reafficher toute l'image (en prenant en compte le facteur de zoom) --

void Display_all_screen(void)
{
  word width;
  word height;

  // ---/\/\/\  Partie non zoom�e: /\/\/\---
  if (Main_magnifier_mode)
  {
    if (Main_image_width<Main_separator_position)
      width=Main_image_width;
    else
      width=Main_separator_position;
  }
  else
  {
    if (Main_image_width<Screen_width)
      width=Main_image_width;
    else
      width=Screen_width;
  }
  if (Main_image_height<Menu_Y)
    height=Main_image_height;
  else
    height=Menu_Y;
  Display_screen(width,height,Main_image_width);

  // Effacement de la partie non-image dans la partie non zoom�e:
  if (Main_magnifier_mode)
  {
    if (Main_image_width<Main_separator_position && Main_image_width < Screen_width)
      Block(Main_image_width,0,(Main_separator_position-Main_image_width),Menu_Y,Main_backups->Pages->Transparent_color);
  }
  else
  {
    if (Main_image_width<Screen_width)
      Block(Main_image_width,0,(Screen_width-Main_image_width),Menu_Y,Main_backups->Pages->Transparent_color);
  }
  if (Main_image_height<Menu_Y)
    Block(0,Main_image_height,width,(Menu_Y-height),Main_backups->Pages->Transparent_color);

  // ---/\/\/\  Partie zoom�e: /\/\/\---
  if (Main_magnifier_mode)
  {
    // Affichage de la barre de split
    Display_separator();

    // Calcul de la largeur visible
    if (Main_image_width<Main_magnifier_width)
      width=Main_image_width;
    else
      width=Main_magnifier_width;

    // Calcul du nombre de lignes visibles de l'image zoom�e
    if (Main_image_height<Main_magnifier_height)
      height=Main_image_height*Main_magnifier_factor;
    else if (Main_image_height<Main_magnifier_offset_Y+Main_magnifier_height)
      // Omit "last line" if it's outside picture limits
      height=Menu_Y/Main_magnifier_factor*Main_magnifier_factor;
    else
      height=Menu_Y;
    
    Display_zoomed_screen(width,height,Main_image_width,Horizontal_line_buffer);

    // Effacement de la partie non-image dans la partie zoom�e:
    if (Main_image_width<Main_magnifier_width)
      Block(Main_X_zoom+(Main_image_width*Main_magnifier_factor),0,
            (Main_magnifier_width-Main_image_width)*Main_magnifier_factor,
            Menu_Y,Main_backups->Pages->Transparent_color);
    if (height<Menu_Y)
      Block(Main_X_zoom,height,width*Main_magnifier_factor,(Menu_Y-height),Main_backups->Pages->Transparent_color);
  }

  // ---/\/\/\ Affichage des limites /\/\/\---
  if (Config.Display_image_limits)
    Display_image_limits();
  Update_rect(0,0,Screen_width,Menu_Y); // TODO On peut faire plus fin, en �vitant de mettre � jour la partie � droite du split quand on est en mode loupe. Mais c'est pas vraiment int�ressant ?
}



byte Best_color(byte r,byte g,byte b)
{
  int col;
  int   delta_r,delta_g,delta_b;
  int   dist;
  int   best_dist=0x7FFFFFFF;
  int   rmean;
  byte  best_color=0;

  for (col=0; col<256; col++)
  {
    if (!Exclude_color[col])
    {
      delta_r=(int)Main_palette[col].R-r;
      delta_g=(int)Main_palette[col].G-g;
      delta_b=(int)Main_palette[col].B-b;

      rmean = ( Main_palette[col].R + r ) / 2;

      if (!(dist= ( ( (512+rmean) *delta_r*delta_r) >>8) + 4*delta_g*delta_g + (((767-rmean)*delta_b*delta_b)>>8)))
      //if (!(dist=(delta_r*delta_r*30)+(delta_g*delta_g*59)+(delta_b*delta_b*11)))
        return col;

      if (dist<best_dist)
      {
        best_dist=dist;
        best_color=col;
      }
    }
  }

  return best_color;
}

byte Best_color_nonexcluded(byte red,byte green,byte blue)
{
  int   col;  
  int   delta_r,delta_g,delta_b;
  int   dist;
  int   best_dist=0x7FFFFFFF;
  int   rmean;
  byte  best_color=0;

  for (col=0; col<256; col++)
  {
    delta_r=(int)Main_palette[col].R-red;
    delta_g=(int)Main_palette[col].G-green;
    delta_b=(int)Main_palette[col].B-blue;

    if(delta_r == 0 && delta_g == 0 && delta_b == 0) return col;

    rmean = ( Main_palette[col].R + red ) / 2;

    dist= ( ( (512+rmean) *delta_r*delta_r) >>8) + 4*delta_g*delta_g + (((767-rmean)*delta_b*delta_b)>>8);
    //dist=(delta_r*delta_r*30)+(delta_g*delta_g*59)+(delta_b*delta_b*11)

    if (dist<best_dist)
    {
      best_dist=dist;
      best_color=col;
    }
  }
  return best_color;
}

byte Best_color_range(byte r, byte g, byte b, byte max)
{
  
  int col;  
  float best_diff=255.0*1.56905;
  byte  best_color=0;
  float target_bri;
  float bri;
  float diff_b, diff_c, diff;

  // Similar to Perceptual_lightness();
  target_bri = sqrt(0.26*r*0.26*r + 0.55*g*0.55*g + 0.19*b*0.19*b);
  
  for (col=0; col<max; col++)
  {
    if (Exclude_color[col])
      continue;

    diff_c = sqrt(
      (0.26*(Main_palette[col].R-r))*
      (0.26*(Main_palette[col].R-r))+
      (0.55*(Main_palette[col].G-g))*
      (0.55*(Main_palette[col].G-g))+
      (0.19*(Main_palette[col].B-b))*
      (0.19*(Main_palette[col].B-b)));
    // Exact match
    if (diff_c==0)
      return col;

    bri = sqrt(0.26*Main_palette[col].R*0.26*Main_palette[col].R + 0.55*Main_palette[col].G*0.55*Main_palette[col].G + 0.19*Main_palette[col].B*0.19*Main_palette[col].B);
    diff_b = abs(target_bri-bri);

    diff=0.25*(diff_b-diff_c)+diff_c;
    if (diff<best_diff)
    {
      best_diff=diff;
      best_color=col;
    } 
  }

  return best_color;
}

byte Best_color_perceptual(byte r,byte g,byte b)
{
  
  int col;  
  float best_diff=255.0*1.56905;
  byte  best_color=0;
  float target_bri;
  float bri;
  float diff_b, diff_c, diff;

  // Similar to Perceptual_lightness();
  target_bri = sqrt(0.26*r*0.26*r + 0.55*g*0.55*g + 0.19*b*0.19*b);
  
  for (col=0; col<256; col++)
  {
    if (Exclude_color[col])
      continue;

    diff_c = sqrt(
      (0.26*(Main_palette[col].R-r))*
      (0.26*(Main_palette[col].R-r))+
      (0.55*(Main_palette[col].G-g))*
      (0.55*(Main_palette[col].G-g))+
      (0.19*(Main_palette[col].B-b))*
      (0.19*(Main_palette[col].B-b)));
    // Exact match
    if (diff_c==0)
      return col;

    bri = sqrt(0.26*Main_palette[col].R*0.26*Main_palette[col].R + 0.55*Main_palette[col].G*0.55*Main_palette[col].G + 0.19*Main_palette[col].B*0.19*Main_palette[col].B);
    diff_b = abs(target_bri-bri);

    diff=0.25*(diff_b-diff_c)+diff_c;
    if (diff<best_diff)
    {
      best_diff=diff;
      best_color=col;
    } 
  }

  return best_color;
}

byte Best_color_perceptual_except(byte r,byte g,byte b, byte except)
{
  
  int col;  
  float best_diff=255.0*1.56905;
  byte  best_color=0;
  float target_bri;
  float bri;
  float diff_b, diff_c, diff;

  // Similar to Perceptual_lightness();
  target_bri = sqrt(0.26*r*0.26*r + 0.55*g*0.55*g + 0.19*b*0.19*b);
  
  for (col=0; col<256; col++)
  {
    if (col==except || Exclude_color[col])
      continue;

    diff_c = sqrt(
      (0.26*(Main_palette[col].R-r))*
      (0.26*(Main_palette[col].R-r))+
      (0.55*(Main_palette[col].G-g))*
      (0.55*(Main_palette[col].G-g))+
      (0.19*(Main_palette[col].B-b))*
      (0.19*(Main_palette[col].B-b)));
    // Exact match
    if (diff_c==0)
      return col;

    bri = sqrt(0.26*Main_palette[col].R*0.26*Main_palette[col].R + 0.55*Main_palette[col].G*0.55*Main_palette[col].G + 0.19*Main_palette[col].B*0.19*Main_palette[col].B);
    diff_b = abs(target_bri-bri);

    diff=0.25*(diff_b-diff_c)+diff_c;
    if (diff<best_diff)
    {
      best_diff=diff;
      best_color=col;
    } 
  }

  return best_color;
}

byte Old_black;
byte Old_dark;
byte Old_light;
byte Old_white;
byte Old_trans;

void Remap_pixel(byte * pixel)
{
  if (*pixel==Old_light)         // On commence par tester le Gris clair
    *pixel=MC_Light;             // qui est pas mal utilis�.
  else
  {
    if (*pixel==Old_black)        // Puis le Noir...
      *pixel=MC_Black;
    else
    {
      if (*pixel==Old_dark)     // etc...
        *pixel=MC_Dark;
      else
      {
        if (*pixel==Old_white)
          *pixel=MC_White;
        else
        {
          if (*pixel==Old_trans)
            *pixel=MC_Trans;
        }
      }
    }
  }
}



void Remap_screen_after_menu_colors_change(void)
{
  short index;
  byte  conversion_table[256];
  short temp/*,temp2*/;

  if ( (MC_Light!=Old_light) || (MC_Dark!=Old_dark) || (MC_White!=Old_white) || (MC_Black !=Old_black )
    || (MC_Trans!=Old_trans) )
  {
    // Cr�ation de la table de conversion
    for (index=0; index<256; index++)
      conversion_table[index]=index;

    conversion_table[Old_black ]=MC_Black;
    conversion_table[Old_dark]=MC_Dark;
    conversion_table[Old_light]=MC_Light;
    conversion_table[Old_white]=MC_White;

    // Remappage de l'�cran

    temp=Window_height*Menu_factor_Y;

    Remap_screen(Window_pos_X, Window_pos_Y,
                 Window_width*Menu_factor_X,
                 (Window_pos_Y+temp<Menu_Y_before_window)?temp:Menu_Y_before_window-Window_pos_Y,
                 conversion_table);

    if (Menu_is_visible_before_window)
    {
      Remap_screen(0, Menu_Y_before_window,
                   Screen_width, Screen_height-Menu_Y_before_window,
                   conversion_table);
      // Remappage de la partie du fond de la fenetre qui cacherait le menu...
      Remap_window_backgrounds(conversion_table, Menu_Y_before_window, Screen_height);
      /*
         Il faudrait peut-�tre remapper les pointill�s d�limitant l'image.
         Mais �a va �tre chiant parce qu'ils peuvent �tre affich�s en mode Loupe.
         Mais de toutes fa�ons, c'est franchement facultatif...
      */
      // On passe la table juste pour ne rafficher que les couleurs modifi�es
      Display_menu_palette_avoiding_window(conversion_table);
    }
    Clear_border(MC_Black);
  }

}


static int Diff(int i, int j) {
	int dr = Main_palette[i].R - Main_palette[j].R;
	int dg = Main_palette[i].G - Main_palette[j].G;
	int db = Main_palette[i].B - Main_palette[j].B;

	return dr*dr + dg*dg + db*db;
}

static void compute_xor_table()
{
	int i;
	byte found;

	// Initialize the table with some "random" values
	for(i = 0; i < 256; i++)
	{
		xor_lut[i] = i ^ 1;
	}

	do {
		// Find the smallest difference in the table
		int idx;

		// Try to pair these two colors better
		found = 0;
		for(idx = 0; idx < 256; idx++)
		{
			int improvement = 0;
			int betterpair = idx;
			for(i = 0; i < 256; i++)
			{
				// diffs before the swap
				int before = Diff(idx, xor_lut[idx]) + Diff(i, xor_lut[i]);

				// diffs after the swap
				int after = Diff(idx, xor_lut[i])  + Diff(i, xor_lut[idx]);

				if (after - before > improvement)
				{
					improvement = after - before;
					betterpair = i;
				}
			}

			if (improvement > 0)
			{
				// Swapping these colors get us something "more different". Do it !
				byte idx2 = xor_lut[betterpair];
				byte i2 = xor_lut[idx];

				xor_lut[betterpair] = i2;
				xor_lut[i2] = betterpair;
				xor_lut[idx] = idx2;
				xor_lut[idx2] = idx;

				found = 1;
			}
		}
	} while(found);
}

int Same_color(T_Components * palette, byte c1, byte c2)
{
	if (palette[c1].R==palette[c2].R &&
			palette[c1].G==palette[c2].G &&
			palette[c1].B==palette[c2].B)
		return 1;
	return 0;
}

void Compute_optimal_menu_colors(T_Components * palette)
{
	int i;
	byte l[256];
	byte s[256];
	byte h;
	int max_l = -1, min_l = 256;
	int low_l, hi_l;
	int delta_low = 999999;
	int delta_high = 999999;
	const int tolerence=16;
	const T_Components cpc_colors[4] = {
		{  0,  0,  0},
		{  0,  0,128}, // Dark blue
		{128,128,128}, // Grey
		{255,255,255}
	};

	Old_black =MC_Black;
	Old_dark = MC_Dark;
	Old_light = MC_Light;
	Old_white = MC_White;
	Old_trans = MC_Trans;

	// First method:
	// If all close matches for the ideal colors exist, pick them.
	for (i=255; i>=0; i--)
	{

		if (Round_palette_component(palette[i].R)/tolerence==Gfx->Default_palette[Gfx->Color[3]].R/tolerence
				&& Round_palette_component(palette[i].G)/tolerence==Gfx->Default_palette[Gfx->Color[3]].G/tolerence
				&& Round_palette_component(palette[i].B)/tolerence==Gfx->Default_palette[Gfx->Color[3]].B/tolerence)
		{
			MC_White=i;
			for (i=255; i>=0; i--)
			{
				if (Round_palette_component(palette[i].R)/tolerence==Gfx->Default_palette[Gfx->Color[2]].R/tolerence
						&& Round_palette_component(palette[i].G)/tolerence==Gfx->Default_palette[Gfx->Color[2]].G/tolerence
						&& Round_palette_component(palette[i].B)/tolerence==Gfx->Default_palette[Gfx->Color[2]].B/tolerence)
				{
					MC_Light=i;
					for (i=255; i>=0; i--)
					{
						if (Round_palette_component(palette[i].R)/tolerence==Gfx->Default_palette[Gfx->Color[1]].R/tolerence
								&& Round_palette_component(palette[i].G)/tolerence==Gfx->Default_palette[Gfx->Color[1]].G/tolerence
								&& Round_palette_component(palette[i].B)/tolerence==Gfx->Default_palette[Gfx->Color[1]].B/tolerence)
						{
							MC_Dark=i;
							for (i=255; i>=0; i--)
							{
								if (Round_palette_component(palette[i].R)/tolerence==Gfx->Default_palette[Gfx->Color[0]].R/tolerence
										&& Round_palette_component(palette[i].G)/tolerence==Gfx->Default_palette[Gfx->Color[0]].G/tolerence
										&& Round_palette_component(palette[i].B)/tolerence==Gfx->Default_palette[Gfx->Color[0]].B/tolerence)
								{
									MC_Black=i;
									// On cherche une couleur de transparence diff�rente des 4 autres.
									for (MC_Trans=0; ((MC_Trans==MC_Black) || (MC_Trans==MC_Dark) ||
												(MC_Trans==MC_Light) || (MC_Trans==MC_White)); MC_Trans++);
									// Easy case
									MC_OnBlack=MC_Dark;
									MC_Window=MC_Light;
									MC_Lighter=MC_White;
									MC_Darker=MC_Dark;
									Remap_menu_sprites();
									return;
								}
							}
            }
          }
        }
      }
    }
  }
  // Second method: For CPC 27-color modes only
  // Try to find colors that just work
  if (Get_palette_RGB_scale()==3)
  for (i=255; i>=0; i--)
  {
    
    if (Round_palette_component(palette[i].R)/tolerence==cpc_colors[3].R/tolerence
     && Round_palette_component(palette[i].G)/tolerence==cpc_colors[3].G/tolerence
     && Round_palette_component(palette[i].B)/tolerence==cpc_colors[3].B/tolerence)
    {
      MC_White=i;
      for (i=255; i>=0; i--)
      {
        if (Round_palette_component(palette[i].R)/tolerence==cpc_colors[2].R/tolerence
         && Round_palette_component(palette[i].G)/tolerence==cpc_colors[2].G/tolerence
         && Round_palette_component(palette[i].B)/tolerence==cpc_colors[2].B/tolerence)
        {
          MC_Light=i;
          for (i=255; i>=0; i--)
          {
            if (Round_palette_component(palette[i].R)/tolerence==cpc_colors[1].R/tolerence
             && Round_palette_component(palette[i].G)/tolerence==cpc_colors[1].G/tolerence
             && Round_palette_component(palette[i].B)/tolerence==cpc_colors[1].B/tolerence)
            {
              MC_Dark=i;
              for (i=255; i>=0; i--)
              {
                if (Round_palette_component(palette[i].R)/tolerence==cpc_colors[0].R/tolerence
                 && Round_palette_component(palette[i].G)/tolerence==cpc_colors[0].G/tolerence
                 && Round_palette_component(palette[i].B)/tolerence==cpc_colors[0].B/tolerence)
                {
                  MC_Black=i;
                  // On cherche une couleur de transparence diff�rente des 4 autres.
                  for (MC_Trans=0; ((MC_Trans==MC_Black) || (MC_Trans==MC_Dark) ||
                                   (MC_Trans==MC_Light) || (MC_Trans==MC_White)); MC_Trans++);
                  // Easy case
                  MC_OnBlack=MC_Dark;
                  MC_Window=MC_Light;
                  MC_Lighter=MC_White;
                  MC_Darker=MC_Dark;
                  Remap_menu_sprites();
                  return;
                }
              }
            }
          }
        }
      }
    }
  }
  
  // Third method:

  // Compute luminance for whole palette
  // Take the darkest as black, the brightest white
  for(i = 0; i < 256; i++)
  {
    RGB_to_HSL(palette[i].R, palette[i].G, palette[i].B, &h, &s[i], &l[i]);
    // Another formula for lightness, in 0-255 range
    //l[i]=Perceptual_lightness(&palette[i])/4062/255;
    if (l[i] > max_l)
    {
      max_l = l[i];
      MC_White = i;
    }
  }
  for(i = 0; i < 256; i++)
  {
    if (l[i] < min_l && i!=MC_White)
    {
      min_l = l[i];
      MC_Black = i;
    }
  }
  // Alter the S values according to the L range - this is for the future
  // comparisons, so that highly variable saturation doesn't weigh
  // too heavily when the the lightness is in a narrow range.
  for(i = 0; i < 256; i++)
  {
    s[i]=s[i]*(max_l-min_l)/255;
  }
  for(i = 0; i < 256; i++)
  {
    // Adjust (reduce) perceived saturation at both ends of L spectrum
    if (l[i]>192)
      s[i]=s[i]*(255-l[i])/64;
    else if (l[i]<64)
      s[i]=s[i]*l[i]/64;
  }
  

  // Find color nearest to min+2(max-min)/3
  // but at the same time we try to minimize the saturation so that the menu
  // still looks grey
  hi_l = min_l + 2*(max_l - min_l)/3;

  for (i = 0; i < 256; i++)
  {
    if ( abs(l[i] - hi_l) + s[i]/2 < delta_high && i!=MC_White && i!=MC_Black)
    {
      delta_high = abs(l[i] - hi_l) + s[i]/2;
      MC_Light = i;
    }
  }
  
  // Target "Dark color" is 2/3 between Light and Black
  low_l = ((int)l[MC_Light]*2+l[MC_Black])/3;
  for (i = 0; i < 256; i++)
  {
    if ( abs((int)l[i] - low_l) + s[i]/6 < delta_low && i!=MC_White && i!=MC_Black && i!=MC_Light)
    {
      delta_low = abs((int)l[i] - low_l)+ s[i]/6;
      MC_Dark = i;
    }
  }
  
  
  //if (l[MC_Light]<l[MC_Dark])
  //{
  //  // Not sure if that can happen, but just in case:
  //  SWAP_BYTES(MC_Light, MC_Dark)
  //}

  // Si deux des couleurs choisies ont le m�me index, c'est destructif car 
  // on fait ensuite un remap de l'image. Donc on �vite ce probl�me (un
  // peu brutalement)
  // On commence par d�placer les gris, comme �a on a plus de chances de garder au moins
  // le blanc et le noir
  //while (MC_Dark == MC_Light || MC_Dark == MC_White || MC_Black == MC_Dark || Same_color(palette, MC_Dark, MC_White)) MC_Dark--;
  //while (MC_White == MC_Light || MC_Dark == MC_Light || MC_Black == MC_Light || Same_color(palette, MC_Light, MC_Black)) MC_Light--;
  //while (MC_White == MC_Light || MC_Dark == MC_White || MC_Black == MC_White) MC_White--;

  // On cherche une couleur de transparence diff�rente des 4 autres.
  for (MC_Trans=0; ((MC_Trans==MC_Black) || (MC_Trans==MC_Dark) ||
                   (MC_Trans==MC_Light) || (MC_Trans==MC_White)); MC_Trans++);
  
  if (Same_color(palette, MC_Black, MC_Dark))
    MC_OnBlack=MC_Light;
  else
    MC_OnBlack=MC_Dark;
  
  if (Same_color(palette, MC_White, MC_Light))
  {
    MC_Window=MC_Dark;
    MC_Darker=MC_Black;
  }
  else
  {
    MC_Window=MC_Light;
    MC_Darker=MC_Dark;
  }
  MC_Lighter=MC_White;
  
  Remap_menu_sprites();
}

/// Remap all menu data when the palette changes or a new skin is loaded
void Remap_menu_sprites()
{
  int i, j, k, l;

  compute_xor_table();
  if ( (MC_Light!=Old_light)
    || (MC_Dark!=Old_dark)
    || (MC_White!=Old_white)
    || (MC_Black !=Old_black )
    || (MC_Trans!=Old_trans) )
  {
    // Mouse cursor sprites
    for (k=0; k<NB_CURSOR_SPRITES; k++)
      for (j=0; j<CURSOR_SPRITE_HEIGHT; j++)
        for (i=0; i<CURSOR_SPRITE_WIDTH; i++)
          Remap_pixel(&Gfx->Cursor_sprite[k][j][i]);
    // Main menu bar
    for (k=0; k<3; k++)
      for (j=0; j<Menu_bars[MENUBAR_TOOLS].Height; j++)
        for (i=0; i<Menu_bars[MENUBAR_TOOLS].Skin_width; i++)
          Remap_pixel(&Gfx->Menu_block[k][j][i]);
    // Menu sprites
    for (l=0; l<2; l++)
      for (k=0; k<NB_MENU_SPRITES; k++)
        for (j=0; j<MENU_SPRITE_HEIGHT; j++)
          for (i=0; i<MENU_SPRITE_WIDTH; i++)
            Remap_pixel(&Gfx->Menu_sprite[l][k][j][i]);
    // Effects sprites
    for (k=0; k<NB_EFFECTS_SPRITES; k++)
      for (j=0; j<EFFECT_SPRITE_HEIGHT; j++)
        for (i=0; i<EFFECT_SPRITE_WIDTH; i++)
          Remap_pixel(&Gfx->Effect_sprite[k][j][i]);
    // Layers buttons
    for (l=0; l<3; l++)
      for (k=0; k<16; k++)
        for (j=0; j<LAYER_SPRITE_HEIGHT; j++)
          for (i=0; i<LAYER_SPRITE_WIDTH; i++)
            Remap_pixel(&Gfx->Layer_sprite[l][k][j][i]);
    
    // Status bar
    for (k=0; k<3; k++)
      for (j=0; j<Menu_bars[MENUBAR_STATUS].Height; j++)
        for (i=0; i<Menu_bars[MENUBAR_STATUS].Skin_width; i++)
          Remap_pixel(&Gfx->Statusbar_block[k][j][i]);
    // Layer bar
    for (k=0; k<3; k++)
      for (j=0; j<10; j++)
        for (i=0; i<144; i++)
          Remap_pixel(&Gfx->Layerbar_block[k][j][i]);
    // Anim bar
    for (k=0; k<3; k++)
      for (j=0; j<14; j++)
        for (i=0; i<236; i++)
          Remap_pixel(&Gfx->Animbar_block[k][j][i]);
    // Help fonts
    for (k=0; k<256; k++)
      for (j=0; j<8; j++)
        for (i=0; i<6; i++)
          Remap_pixel(&Gfx->Help_font_norm[k][i][j]);
    for (k=0; k<256; k++)
      for (j=0; j<8; j++)
        for (i=0; i<6; i++)
          Remap_pixel(&Gfx->Bold_font[k][i][j]);
    for (k=0; k<64; k++)
      for (j=0; j<8; j++)
        for (i=0; i<6; i++)
          Remap_pixel(&Gfx->Help_font_t1[k][i][j]);
    for (k=0; k<64; k++)
      for (j=0; j<8; j++)
        for (i=0; i<6; i++)
          Remap_pixel(&Gfx->Help_font_t2[k][i][j]);
    for (k=0; k<64; k++)
      for (j=0; j<8; j++)
        for (i=0; i<6; i++)
          Remap_pixel(&Gfx->Help_font_t3[k][i][j]);
    for (k=0; k<64; k++)
      for (j=0; j<8; j++)
        for (i=0; i<6; i++)
          Remap_pixel(&Gfx->Help_font_t4[k][i][j]);
        
    // Drives and other misc. 8x8 icons
    for (k=0; k<NB_ICON_SPRITES; k++)
      for (j=0; j<ICON_SPRITE_HEIGHT; j++)
        for (i=0; i<ICON_SPRITE_WIDTH; i++)
          Remap_pixel(&Gfx->Icon_sprite[k][j][i]);

    // Skin preview
    for (j = 0; j < 173; j++)
      for (i = 0; i < 16; i++)
        Remap_pixel(&Gfx->Preview[i][j]);
  }
  Clear_border(MC_Black);
}
