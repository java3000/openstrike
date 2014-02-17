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

#include <cmath>

#include <game/game.hh>
#include <game/visitor.hh>

#include <gameobjects/bullet.hh>
#include <gameobjects/rocket.hh>

#include <gameobjects/heli.hh>

constexpr Vector3f Heli::gun_offset_;

Heli::Heli(Game& game) : GameObject(game) {
	age_ = 0;

	pos_.z = maxheight_;

	guns_ = gun_capacity_;
	hydras_ = hydra_capacity_;
	hellfires_ = hellfire_capacity_;

	armor_ = armor_capacity_;
	fuel_ = fuel_capacity_;
	load_ = 0;

	gun_reload_ = hydra_reload_ = hellfire_reload_ = 0; // ready to fire

	control_flags_ = tick_control_flags_ = 0;
}

void Heli::Accept(Visitor& visitor) {
	visitor.Visit(*this);
}

void Heli::Update(unsigned int deltams) {
	UpdatePhysics(deltams);
	UpdateWeapons(deltams);

	// Post-update
	tick_control_flags_ = 0;

	age_ += deltams;
}

void Heli::UpdatePhysics(unsigned int deltams) {
	float delta_sec = deltams / 1000.0f;

	// Turning
	float turn_rate = (control_flags_ & FORWARD) ? accel_turn_rate_ : still_turn_rate_;

	if (control_flags_ & LEFT)
		direction_.RotateCCW(turn_rate * delta_sec);
	if (control_flags_ & RIGHT)
		direction_.RotateCW(turn_rate * delta_sec);

	// XXX: acceleration

	// XXX: position
	pos_ += vel_ * delta_sec;
}

void Heli::UpdateWeapons(unsigned int deltams) {
	// take key taps into account
	int combiled_control_flags = control_flags_ | tick_control_flags_;

	// Cooldown
	if (gun_reload_ > 0)
		gun_reload_ -= deltams;
	if (hydra_reload_ > 0)
		hydra_reload_ -= deltams;
	if (hellfire_reload_ > 0)
		hellfire_reload_ -= deltams;

	Direction2f firedir((int)((direction_.yaw / pi * 12.0) + 0.5) * pi / 12.0);

	// Process gunfire
	if (combiled_control_flags & GUN && guns_ > 0 && gun_reload_ <= 0) {
		game_.Spawn<Bullet>(pos_ + gun_offset_ * firedir, Direction3f(firedir, weapon_fire_pitch_));

		guns_--;
		gun_reload_ = gun_cooldown_;
	}

	if (combiled_control_flags & HYDRA && hydras_ > 0 && hydra_reload_ <= 0) {
		// XXX: swapn at sides of helicopter
		game_.Spawn<Rocket>(pos_ + gun_offset_ * firedir, Direction3f(firedir, weapon_fire_pitch_), Rocket::HYDRA);

		hydras_--;
		hydra_reload_ = hydra_cooldown_;
	}

	if (tick_control_flags_ /* hellfires have no autofire */ & HELLFIRE && hellfires_ > 0 && hellfire_reload_ <= 0) {
		// XXX: swapn at sides of helicopter
		game_.Spawn<Rocket>(pos_ + gun_offset_ * firedir, Direction3f(firedir, weapon_fire_pitch_), Rocket::HELLFIRE);

		hellfires_--;
		hellfire_reload_ = hellfire_cooldown_;
	}

	// XXX: no-ammo case should emit clicking sound; this
	// probably should be done via emitting some kind of event
}
