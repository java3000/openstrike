/*
 * Copyright (C) 2013-2014 Dmitry Marakasov
 *
 * This file is part of openstrike.
 *
 * openstrike is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openstrike is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openstrike.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <set>
#include <cassert>
#include <iostream>

#include <dat/datlevel.hh>

// this is stored in the code; not sure if it'd be better to extract
// if from there instead of just keeping this table here
const std::map<unsigned short, std::string> DatLevel::gfx_resources_ = {
	{ 0xb024, "HANGAR" },
	{ 0xb026, "COMMAND" },
	{ 0xb028, "MIG23" },
	{ 0xb02a, "FRIGATE" },
	{ 0xb02c, "BARKS" },
	{ 0xb02e, "BLDG1" },
	{ 0xb032, "BLDGS23" },
	{ 0xb034, "BRADLEY" },
	{ 0xb038, "BUNKER" },
	{ 0xb03a, "CRATERS" },
	{ 0xb03c, "FENCE" },
	{ 0xb03e, "GUARDS" },
	{ 0xb042, "NOMADS" },
	{ 0xb044, "PWIRES" },
	{ 0xb046, "PWPOLES" },
	{ 0xb048, "RESCUEBO" },
	{ 0xb04a, "ROADS" },
	{ 0xb04c, "ROCKS" },
	{ 0xb050, "SUBSTATI" },
	{ 0xb052, "TENTS" },
	{ 0xb054, "TOWER" },
	{ 0xb064, "JAIL" },
	{ 0xb066, "NMEHQ" },
	{ 0xb068, "POW" },
	{ 0xb06e, "BLDG6" },
};

DatLevel::DatLevel(const MemRange& leveldata, const MemRange& thingsdata, int width_blocks, int height_blocks) {
	//
	// First block table encodes buildings
	//
	int last_offset = 0;
	int table_offset = last_offset;
	for (int nblock = 0; nblock < width_blocks * height_blocks; nblock++) {
		int blockdata_offset = leveldata.GetWord(table_offset + nblock * 2);
		int data_count = leveldata.GetWord(blockdata_offset);

		if (blockdata_offset == 0)
			continue;

		last_offset = blockdata_offset + 2 + data_count * 2;
		for (int ndata = 0; ndata < data_count; ndata++) {
			int data_offset = leveldata.GetWord(blockdata_offset + 2 + 2 * ndata);

			BuildingInstance obj;
			obj.type = leveldata.GetWord(data_offset);
			obj.sprite_y = leveldata.GetSWord(data_offset + 2) * 8;
			obj.sprite_x = leveldata.GetSWord(data_offset + 4) * 8;
			obj.y = leveldata.GetWord(data_offset + 6);
			obj.x = leveldata.GetWord(data_offset + 8);

			if (data_offset != last_offset) {
				assert(data_offset == last_offset + 6);
				obj.dead_type = leveldata.GetWord(last_offset);
				obj.dead_sprite_y = leveldata.GetSWord(data_offset + 2) * 8;
				obj.dead_sprite_x = leveldata.GetSWord(data_offset + 4) * 8;
			}

			// 18 bytes of mandatory data is followed
			// by chunks which presumably encode what
			// happens when the building is destroyed
			// (e.g. fuel pickup or enemy appears nearby)
			int effect_offset = data_offset + 18;
			while (1) {
				//int effect_type = leveldata.GetByte(effect_offset);
				int effect_data_len = leveldata.GetByte(effect_offset+1);

				if (effect_data_len == 0)
					break;

				effect_offset += effect_data_len + 2;
			}
			last_offset = effect_offset + 2;

			building_instances_.emplace_back(obj);

			building_types_.emplace(std::make_pair(obj.type, BuildingType()));
			if (obj.dead_type)
				building_types_.emplace(std::make_pair(obj.dead_type, BuildingType()));
		}
	}

	//
	// Second block table encodes units
	//
	table_offset = last_offset;
	for (int nblock = 0; nblock < width_blocks * height_blocks; nblock++) {
		int blockdata_offset = leveldata.GetWord(table_offset + nblock * 2);
		int data_count = leveldata.GetWord(blockdata_offset);

		if (blockdata_offset == 0)
			continue;

		last_offset = blockdata_offset + 2 + data_count * 2;
		for (int ndata = 0; ndata < data_count; ndata++) {
			int data_offset = leveldata.GetWord(blockdata_offset + 2 + 2 * ndata);

			UnitInstance obj;
			obj.y = leveldata.GetWord(data_offset + 6);
			obj.x = leveldata.GetWord(data_offset + 8);
			obj.z = leveldata.GetSWord(data_offset + 10);

			// 20 bytes of mandatory data is followed
			// by chunks which presumably encode what
			// happens when the unit is destroyed
			// (e.g. objective is counted complete)
			int effect_offset = data_offset + 20;
			while (1) {
				int effect_data_len = leveldata.GetByte(effect_offset+1);

				if (effect_data_len == 0)
					break;

				effect_offset += effect_data_len + 2;
			}
			last_offset = effect_offset + 2;

			unit_instances_.emplace_back(obj);

			// TODO: create unit type
		}
	}

	// Check that level file was read completely
	assert((size_t)last_offset == leveldata.GetSize());

	//
	// Load extra data from other files (buildings)
	//
	for (auto& type : building_types_) {
		unsigned short blocks_identifier = thingsdata.GetWord(type.first);

		type.second.width = thingsdata.GetWord(type.first + 2);
		type.second.height = thingsdata.GetWord(type.first + 4);
		type.second.health = thingsdata.GetWord(type.first + 12);

		auto resource = gfx_resources_.find(blocks_identifier);
		type.second.resource_name = (resource == gfx_resources_.end()) ? "" : resource->second;

		if (resource == gfx_resources_.end()) {
			std::cerr << "Warning: no resource name for object type " << blocks_identifier << std::endl;
		}

		int block_matrix_offset = thingsdata.GetWord(type.first + 6);

		for (int nblock = 0; nblock < (type.second.width / 16) * (type.second.height / 16); nblock++)
			type.second.blocks.push_back(thingsdata.GetWord(block_matrix_offset + nblock * 2));

		int nbboxes = thingsdata.GetWord(type.first + 20);

		for (int nbbox = 0; nbbox < nbboxes; nbbox++) {
			int bbox_offset = type.first + 22 + nbbox * 12;
			BuildingType::BBox bbox;
			bbox.y1 = thingsdata.GetWord(bbox_offset + 0);
			bbox.x1 = thingsdata.GetWord(bbox_offset + 2);
			bbox.y2 = thingsdata.GetWord(bbox_offset + 4);
			bbox.x2 = thingsdata.GetWord(bbox_offset + 6);
			bbox.z1 = thingsdata.GetWord(bbox_offset + 8);
			bbox.z2 = thingsdata.GetWord(bbox_offset + 10);

			type.second.bboxes.emplace_back(bbox);
		}
	}
}

void DatLevel::ForeachBuildingInstance(const BuildingInstanceProcessor& fn) const {
	for (auto& bi : building_instances_)
		fn(bi);
}

void DatLevel::ForeachBuildingType(const BuildingTypeProcessor& fn) const {
	for (auto& bt : building_types_)
		fn(bt.first, bt.second);
}

void DatLevel::ForeachUnitInstance(const UnitInstanceProcessor& fn) const {
	for (auto& ui : unit_instances_)
		fn(ui);
}

const DatLevel::BuildingType& DatLevel::GetBuildingType(unsigned short type) const {
	auto it = building_types_.find(type);
	if (it == building_types_.end())
		throw std::runtime_error("unknown building type");
	return it->second;
}
