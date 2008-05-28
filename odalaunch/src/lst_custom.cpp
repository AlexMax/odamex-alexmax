// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2006-2008 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Custom list control, featuring sorting
//	AUTHOR:	Russell Rice
//
//-----------------------------------------------------------------------------


#include "lst_custom.h"

IMPLEMENT_DYNAMIC_CLASS(wxAdvancedListCtrl, wxListCtrl)

BEGIN_EVENT_TABLE(wxAdvancedListCtrl, wxListCtrl)
     EVT_LIST_COL_CLICK(-1, wxAdvancedListCtrl::OnHeaderColumnButtonClick)
     EVT_WINDOW_CREATE(wxAdvancedListCtrl::OnCreateControl)
END_EVENT_TABLE()

// this is so we can jump over the sort arrow images
enum
{
    LIST_SORT_ARROW_UP = 0
    ,LIST_SORT_ARROW_DOWN
    ,FIRST_IMAGE
};

// Sorting arrow XPM images
static const char *SortArrowAscending[] =
{
    "16 15 3 1",
    "  c None",
    "0 c #808080",
    "1 c #FFFFFF",
    
    "                ",
    "                ",
    "                ",
    "                ",
    "       01       ",
    "      0011      ",
    "      0  1      ",
    "     00  11     ",
    "     0    1     ",
    "    00    11    ",
    "    01111111    ",
    "                ",
    "                ",
    "                ",
    "                "
};

static const char *SortArrowDescending[] =
{
    "16 15 3 1",
    "  c None",
    "0 c #808080",
    "1 c #FFFFFF",
    
    "                ",
    "                ",
    "                ",
    "                ",
    "    00000000    ",
    "    00    11    ",
    "     0    1     ",
    "     00  11     ",
    "      0  1      ",
    "      0011      ",
    "       01       ",
    "                ",
    "                ",
    "                ",
    "                "
};

void wxAdvancedListCtrl::OnCreateControl(wxWindowCreateEvent &event)
{
    SortOrder = 0; 
    SortCol = 0; 

    colRed = 245;
    colGreen = 245;
    colBlue = 245;

    // Set up the image list.
    AddImageSmall(NULL);
}

// Add any additional bitmaps/icons to the internal image list
void wxAdvancedListCtrl::AddImageSmall(wxImage Image)
{
    if (GetImageList(wxIMAGE_LIST_SMALL) == NULL)
    {
        // Art provider images are 16x15, WTF?! Kept for compatibility :'(
        wxImageList *ImageList = new wxImageList(16, 15, true, FIRST_IMAGE);
        AssignImageList(ImageList, wxIMAGE_LIST_SMALL);
        
        // Add our sort icons by default.
        GetImageList(wxIMAGE_LIST_SMALL)->Add(wxImage(SortArrowAscending));
        GetImageList(wxIMAGE_LIST_SMALL)->Add(wxImage(SortArrowDescending));
    }
    
    if (Image.IsOk())
    {
        GetImageList(wxIMAGE_LIST_SMALL)->Add(Image);
    }
}

// Adjusts the index, so it jumps over the sort arrow images.
void wxAdvancedListCtrl::SetColumnImage(wxListItem &li, wxInt32 ImageIndex)
{
    if (ImageIndex < -1)
        ImageIndex = -1;

    li.SetImage(((ImageIndex == -1) ? ImageIndex : FIRST_IMAGE + ImageIndex));
}

// [Russell] - These are 2 heavily modified routines of the javascript natural 
// compare by Kristof Coomans (it was easier to follow than the original C 
// version by Martin Pool), their versions were under the ZLIB license (which 
// is compatible with the GPL).
//
// Original Javascript version by Kristof Coomans
//      http://sourcefrog.net/projects/natsort/natcompare.js
//
// Do not contact the mentioned authors about my version.
wxInt32 NaturalCompareWorker(const wxString &String1, const wxString &String2)
{
    wxInt32 Direction = 0;

    wxChar String1Char, String2Char;

    for (wxUint32 String1Counter = 0, String2Counter = 0;
         String1.Len() > 0 && String2.Len() > 0; 
         ++String1Counter, ++String2Counter) 
    {
        String1Char = String1[String1Counter];
        String2Char = String2[String2Counter];

        if (!wxIsdigit(String1Char) && !wxIsdigit(String2Char)) 
        {
            return Direction;
        } 
        
        if (!wxIsdigit(String1Char)) 
        {
            return -1;
        } 
        
        if (!wxIsdigit(String2Char)) 
        {
            return 1;
        } 
        
        if (String1Char < String2Char) 
        {
            if (Direction == 0) 
            {
                Direction = -1;
            }
        } 
        
        if (String1Char > String2Char) 
        {
            if (Direction == 0)
            {
                Direction = 1;
            }
        } 
        
        if (String1Char == 0 && String2Char == 0) 
        {
            return Direction;
        }
    }
    
    return 0;
}

wxInt32 NaturalCompare(wxString String1, wxString String2, bool CaseSensitive = false) 
{
    wxInt32 StringCounter1 = 0, StringCounter2 = 0;
	wxInt32 String1Zeroes = 0, String2Zeroes = 0;
	wxChar String1Char, String2Char;
	wxInt32 Result;

    if (!CaseSensitive)
    {
        String1.MakeLower();
        String2.MakeLower();
    }

    while (true)
    {
        String1Zeroes = 0;
        String2Zeroes = 0;

        String1Char = String1[StringCounter1];
        String2Char = String2[StringCounter2];

        // skip past whitespace or zeroes in first string
        while (wxIsspace(String1Char) || String1Char == '0' ) 
        {
            if (String1Char == '0') 
            {
                String1Zeroes++;
            } 
            else 
            {
                String1Zeroes = 0;
            }

            String1Char = String1[++StringCounter1];
        }
        
        // skip past whitespace or zeroes in second string
        while (wxIsspace(String2Char) || String2Char == '0') 
        {
            if (String2Char == '0') 
            {
                String2Zeroes++;
            } 
            else 
            {
                String2Zeroes = 0;
            }

            String2Char = String2[++StringCounter2];
        }

        // We encountered some digits, compare these.
        if (wxIsdigit(String1Char) && wxIsdigit(String2Char)) 
        {
            if ((Result = NaturalCompareWorker(
                String1.Mid(StringCounter1), 
                String2.Mid(StringCounter2))) != 0) 
            {
                return Result;
            }
        }

        if ((String1Char == 0) && (String2Char == 0)) 
        {
            return (String1Zeroes - String2Zeroes);
        }

        if (String1Char < String2Char) 
        {
            return -1;
        } 
        else if (String1Char > String2Char) 
        {
            return 1;
        }

        ++StringCounter1; 
        ++StringCounter2;
    }
}

wxInt32 SortRoutine(wxInt32 Order, wxListItem &Item1, wxListItem &Item2) 
{   
    if (Order == 1) 
        return NaturalCompare(Item1.GetText(), Item2.GetText());

    return NaturalCompare(Item2.GetText(), Item1.GetText());
}

// Makes a row exchange places
void wxAdvancedListCtrl::FlipRow(long Row, long NextRow) 
{ 
    if(Row == NextRow) 
        return; 

    // Retrieve data for the next item
    wxListItem Item1, Item2;
    wxListItem Item1Flipped, Item2Flipped; 
    
    Item1.SetId(Row);
    Item1.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_DATA | wxLIST_MASK_IMAGE);
    
    Item2.SetId(NextRow);
    Item2.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_DATA | wxLIST_MASK_IMAGE); 

    Item1Flipped.SetId(NextRow);
    Item1Flipped.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_DATA | wxLIST_MASK_IMAGE);

    Item2Flipped.SetId(Row);
    Item2Flipped.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_DATA | wxLIST_MASK_IMAGE);

    // Flip the data for columns too.
    for (wxInt32 ColumnCounter = 0; 
         ColumnCounter < GetColumnCount(); 
         ++ColumnCounter) 
    {
        Item1.SetColumn(ColumnCounter);
        GetItem(Item1); 

        Item2.SetColumn(ColumnCounter); 
        GetItem(Item2);

        // Do the flip
        // Set data for the first item
        Item2Flipped.SetImage(Item2.GetImage()); 
        Item2Flipped.SetData(Item2.GetData()); 
        Item2Flipped.SetTextColour(Item2.GetTextColour()); 
        Item2Flipped.SetText(Item2.GetText());

        // Now the second
        Item1Flipped.SetImage(Item1.GetImage()); 
        Item1Flipped.SetData(Item1.GetData()); 
        Item1Flipped.SetTextColour(Item1.GetTextColour()); 
        Item1Flipped.SetText(Item1.GetText());

        // Set them
        Item1Flipped.SetColumn(ColumnCounter);
        SetItem(Item1Flipped);
        
        Item2Flipped.SetColumn(ColumnCounter);
        SetItem(Item2Flipped);
    }

}

// A custom sort routine, we do our own sorting.
void wxAdvancedListCtrl::Sort(wxInt32 Column, wxInt32 Order, wxInt32 Lowest, wxInt32 Highest) 
{ 
    if (Highest == -1) 
        Highest = GetItemCount() - 1; 

    wxInt32 LowSection = Lowest; 
    wxInt32 HighSection = Highest; 

    if (HighSection <= LowSection) 
        return;

    // First item.
    wxListItem Item1Check;
    Item1Check.SetColumn(Column); 
    Item1Check.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_DATA | wxLIST_MASK_IMAGE);

    // Second item
    wxListItem Item2Check;
    Item2Check.SetColumn(Column); 
    Item2Check.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_DATA | wxLIST_MASK_IMAGE);

    // Middle Item
    wxListItem MiddleItem;        
    MiddleItem.SetId((LowSection + HighSection) / 2); 
    MiddleItem.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_DATA | wxLIST_MASK_IMAGE); 
    MiddleItem.SetColumn(Column); 
    GetItem(MiddleItem);

    // Loop through the list until indices cross 
    while (LowSection <= HighSection) 
    {
        Item1Check.SetId(LowSection);
        GetItem(Item1Check);
        
        while ((LowSection <= HighSection) && 
               (SortRoutine(Order, Item1Check, MiddleItem) < 0)) 
        {
            ++LowSection;
            Item1Check.SetId(LowSection);
            GetItem(Item1Check);
        }

        Item2Check.SetId(HighSection);
        GetItem(Item2Check);

        while ((LowSection <= HighSection) && 
               (SortRoutine(Order, Item2Check, MiddleItem) > 0))
        { 
            --HighSection;
            Item2Check.SetId(HighSection);
            GetItem(Item2Check);
        }

        // If the indexes have not crossed
        if (LowSection <= HighSection)
        { 
            // Swap only if the items are not equal 
            if (SortRoutine(Order, Item1Check, Item2Check) != 0)
            {
                FlipRow(LowSection, HighSection);
            }           

            ++LowSection;
            --HighSection;
        } 
    }

    // Recursion magic!

    // Sort the lowest section of the array
    if (Lowest < HighSection) 
        Sort(Column, Order, Lowest, HighSection); 

    // Sort the highest section of the array
    if (Highest > LowSection) 
        Sort(Column, Order, LowSection, Highest);
}

void wxAdvancedListCtrl::Sort()
{
    SetSortArrow(SortCol, SortOrder);

    // prime 'er up
    long item = GetNextItem(-1);
      
    while(item != -1) 
    {                    
        SetItemData(item, item); 
 
        item = GetNextItem(item);
    }

    // sort the list by column
    Sort(SortCol, SortOrder);
}

void wxAdvancedListCtrl::OnHeaderColumnButtonClick(wxListEvent &event)
{
    // invert sort order if need be (ascending/descending)
    if (SortCol != event.GetColumn())
        SortOrder = 1;
    else
        SortOrder = !SortOrder;
    
    // column that needs to be sorted, so the rest of the list
    // can be sorted by it
    SortCol = event.GetColumn();

    Sort();
}

void wxAdvancedListCtrl::ResetSortArrows(void)
{
    wxListItem li;
    li.SetMask(wxLIST_MASK_IMAGE);    
    li.SetImage(-1);

    for (wxInt32 i = 0; i < GetColumnCount(); ++i)
    {
        SetColumn(i, li);        
    }
}

void wxAdvancedListCtrl::SetSortArrow(wxInt32 Column, wxInt32 ArrowState)
{
    // nuke any previously set sort arrows
    ResetSortArrows();

    wxListItem li;
    li.SetMask(wxLIST_MASK_IMAGE);
    li.SetImage(ArrowState);

    SetColumn(SortCol, li);
}

void wxAdvancedListCtrl::ColourListItem(wxListItem &info)
{
    static bool SwapColour = false;
    
    wxColour col;
    wxListItemAttr *ListItemAttr = info.GetAttributes();
    
    // Don't change a background colour we didn't set.
    if (ListItemAttr && ListItemAttr->HasBackgroundColour())
    {
        return;
    }
    
    // light grey coolness
    if (SwapColour)
        col.Set(colRed, colGreen, colBlue);
    else
        col.Set(255, 255, 255);

    SwapColour = !SwapColour;

    info.SetBackgroundColour(col);
}

void wxAdvancedListCtrl::ColourListItem(long item)
{
    wxListItem ListItem;
    ListItem.SetId(item);
    
    ColourListItem(ListItem);
    
    SetItem(ListItem);
}

// recolour the entire list
void wxAdvancedListCtrl::ColourList()
{      
    for (long i = 0; i < GetItemCount(); ++i)
        ColourListItem(i);
}

// Our variation of InsertItem, so we can do magical things!
long wxAdvancedListCtrl::ALCInsertItem(wxListItem &info)
{
    //Sort();

    // TODO: We need to remember this item id, because the last item on the list
    // does not get sorted, we can't move sort either, because then the return 
    // index would be stale.
    info.SetId(InsertItem(info));
   
    ColourListItem(info);

    SetItem(info);

    return info.GetId();
}
