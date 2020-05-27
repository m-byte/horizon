
#include "tool_fix_board.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "document/idocument_package.hpp"
#include "pool/package.hpp"
#include "document/idocument_padstack.hpp"
#include "pool/padstack.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "document/idocument_symbol.hpp"
#include "pool/symbol.hpp"
#include "imp/imp_interface.hpp"
#include "canvas/canvas_gl.hpp"
#include "pool/entity.hpp"
#include "nlohmann/json.hpp"
#include <iostream>

namespace horizon {

ToolFixBoard::ToolFixBoard(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolFixBoard::can_begin()
{
    return true;
}

ToolResponse ToolFixBoard::begin(const ToolArgs &args)
{
    std::cout << "tool fix" << std::endl;
    const auto board = doc.b->get_board();
    std::map<const Junction *, std::set<const UUID*>> junction_connections;
    std::map<int64_t, std::map<int64_t, std::pair<const UUID*, const UUID*>>> pad_locations;
    for(const auto &it : board->tracks) {
        for(const auto &it_ft : {it.second.from, it.second.to}) {
            if(it_ft.is_junc()){
                junction_connections[it_ft.junc].insert(&it.first);
            }
        }
    }
    for(const auto &it : board->packages) {
        for(const auto &it_pad : it.second.package.pads) {
            Placement pad;
            pad.accumulate(it.second.placement);
            pad.accumulate(it_pad.second.placement);
            auto pos = pad.shift;
            std::pair<const UUID*, const UUID*> ppad(&it.first, &it_pad.first);
            pad_locations[pos.x][pos.y] = ppad;
        }
    }
    for (const auto &[key, val] : junction_connections) {
        if(val.size() == 1 && pad_locations.count(key->position.x) && pad_locations[key->position.x].count(key->position.y)) {
            std::cout << "single junction at pad found: " << key->position.x << ", " << key->position.y << std::endl;
            auto track = &board->tracks.at(**val.begin());
            auto pkg = &board->packages.at(*pad_locations[key->position.x][key->position.y].first);
            auto pad = &pkg->package.pads.at(*pad_locations[key->position.x][key->position.y].second);
            if (track->from.is_junc() && track->from.junc == key) {
                track->from.connect(pkg, pad);
            }
            if (track->to.is_junc() && track->to.junc == key) {
                track->to.connect(pkg, pad);
            }
	}
    }

    return ToolResponse::commit();
}
ToolResponse ToolFixBoard::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
