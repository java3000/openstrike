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

#ifndef SPRITEMANAGER_HH
#define SPRITEMANAGER_HH

#include <vector>

#include <SDL2pp/Texture.hh>
#include <SDL2pp/Renderer.hh>

#include <graphics/rectpacker.hh>

class DatGraphics;

class SpriteManager {
protected:
	static const int atlas_page_width_;
	static const int atlas_page_height_;

protected:
	struct SpriteInfo {
		unsigned int width;
		unsigned int height;
		unsigned int xoffset;
		unsigned int yoffset;
		unsigned int framewidth;
		unsigned int frameheight;

		unsigned int atlaspage;
		unsigned int atlasx;
		unsigned int atlasy;
	};

	std::vector<SDL2pp::Texture> atlas_pages_;
	std::vector<SpriteInfo> sprites_;

	SDL2pp::Renderer& renderer_;
	RectPacker rect_packer_;

public:
	enum Flags {
		IMAGECORNER = 0x00,
		IMAGECENTER = 0x01,
		FRAMECORNER = 0x02,
		FRAMECENTER = 0x03,
		PIVOTMASK   = 0x03,

		HFLIP       = 0x04,
	};

public:
	SpriteManager(SDL2pp::Renderer& renderer);
	~SpriteManager();

	unsigned int Add(const DatGraphics& graphics, unsigned int first, unsigned int count = 1);
	void Render(unsigned int id, int x, int y, int flags);
};

#endif // SPRITEMANAGER_HH