-- Tile current selection with pattern inside selection.
-- Author: Andrew Trevorrow (andrewtrevorrow.com), Mar 2016.

local g = gollylib()

local selrect = g.getselrect()
if #selrect == 0 then g.exit("There is no selection.") end

local selpatt = g.getcells(selrect)
if #selpatt == 0 then g.exit("No pattern in selection.") end

-- determine if selpatt is one-state or multi-state
local inc = 2
if (#selpatt & 1) == 1 then inc = 3 end

--------------------------------------------------------------------------------

-- return a rect array with the minimal bounding box of given pattern

function getminbox(cells)
    local len = #cells
    if len < 2 then return {} end
    
    local minx = cells[1]
    local miny = cells[2]
    local maxx = minx
    local maxy = miny
    
    -- ignore padding int if present
    if (inc == 3) and (len % 3 == 1) then len = len - 1 end
    
    for x = 1, len, inc do
        if cells[x] < minx then minx = cells[x] end
        if cells[x] > maxx then maxx = cells[x] end
    end
    for y = 2, len, inc do
        if cells[y] < miny then miny = cells[y] end
        if cells[y] > maxy then maxy = cells[y] end
    end
    
    return {minx, miny, maxx - minx + 1, maxy - miny + 1}
end

--------------------------------------------------------------------------------

function clip_left(cells, left)
    local len = #cells
    local x = 1
    if inc == 3 then
        -- ignore padding int if present
        if len % 3 == 1 then len = len - 1 end
        while x <= len do
            if cells[x] >= left then
                g.setcell(cells[x], cells[x+1], cells[x+2])
            end
            x = x + 3
        end
    else
        while x <= len do
            if cells[x] >= left then
                g.setcell(cells[x], cells[x+1], 1)
            end
            x = x + 2
        end
    end
end

--------------------------------------------------------------------------------

function clip_right(cells, right)
    local len = #cells
    local x = 1
    if inc == 3 then
        -- ignore padding int if present
        if len % 3 == 1 then len = len - 1 end
        while x <= len do
            if cells[x] <= right then
                g.setcell(cells[x], cells[x+1], cells[x+2])
            end
            x = x + 3
        end
    else   
        while x <= len do
            if cells[x] <= right then
                g.setcell(cells[x], cells[x+1], 1)
            end
            x = x + 2
        end
    end
end

--------------------------------------------------------------------------------

function clip_top(cells, top)
    local len = #cells
    local y = 2
    if inc == 3 then
        -- ignore padding int if present
        if len % 3 == 1 then len = len - 1 end
        while y <= len do
            if cells[y] >= top then
                g.setcell(cells[y-1], cells[y], cells[y+1])
            end
            y = y + 3
        end
    else   
        while y <= len do
            if cells[y] >= top then
                g.setcell(cells[y-1], cells[y], 1)
            end
            y = y + 2
        end
    end
end

--------------------------------------------------------------------------------

function clip_bottom(cells, bottom)
    local len = #cells
    local y = 2
    if inc == 3 then
        -- ignore padding int if present
        if len % 3 == 1 then len = len - 1 end
        while y <= len do
            if cells[y] <= bottom then
                g.setcell(cells[y-1], cells[y], cells[y+1])
            end
            y = y + 3
        end
    else   
        while y <= len do
            if cells[y] <= bottom then
                g.setcell(cells[y-1], cells[y], 1)
            end
            y = y + 2
        end
    end
end

--------------------------------------------------------------------------------

-- set selection edges
local selleft = selrect[1]
local seltop = selrect[2]
local selright = selleft + selrect[3] - 1
local selbottom = seltop + selrect[4] - 1

-- find selpatt's minimal bounding box
local bbox = getminbox(selpatt)
local i

-- first tile selpatt horizontally, clipping where necessary
local left = bbox[1]
local right = left + bbox[3] - 1
i = 0
while left > selleft do
    left = left - bbox[3]
    i = i + 1
    if left >= selleft then
        g.putcells(selpatt, -bbox[3] * i, 0)
    else
        local tempcells = g.transform(selpatt, -bbox[3] * i, 0)
        clip_left(tempcells, selleft)
    end
end
i = 0
while right < selright do
    right = right + bbox[3]
    i = i + 1
    if right <= selright then
        g.putcells(selpatt, bbox[3] * i, 0)
    else
        local tempcells = g.transform(selpatt, bbox[3] * i, 0)
        clip_right(tempcells, selright)
    end
end

-- get new selection pattern and tile vertically, clipping where necessary
selpatt = g.getcells(selrect)
bbox = getminbox(selpatt)
local top = bbox[2]
local bottom = top + bbox[4] - 1
i = 0
while top > seltop do
    top = top - bbox[4]
    i = i + 1
    if top >= seltop then
        g.putcells(selpatt, 0, -bbox[4] * i)
    else
        local tempcells = g.transform(selpatt, 0, -bbox[4] * i)
        clip_top(tempcells, seltop)
    end
end
i = 0
while bottom < selbottom do
    bottom = bottom + bbox[4]
    i = i + 1
    if bottom <= selbottom then
        g.putcells(selpatt, 0, bbox[4] * i)
    else
        local tempcells = g.transform(selpatt, 0, bbox[4] * i)
        clip_bottom(tempcells, selbottom)
    end
end

if not g.visrect(selrect) then g.fitsel() end
