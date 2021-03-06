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
#include <cstdlib>

#include <game/game.hh>
#include <game/visitor.hh>

#include <gameobjects/projectile.hh>

#include <gameobjects/heli.hh>

Heli::Heli(Game& game, const Vector2f& pos) : GameObject(game) {
	age_ = 0;

	pos_ = pos;
	pos_.z = Constants::MaxHeight();

	guns_ = Constants::GunCapacity();
	hydras_ = Constants::HydraCapacity();;
	hellfires_ = Constants::HellfireCapacity();;

	hydra_at_left_ = true;
	hellfire_at_left_ = true;

	armor_ = Constants::ArmorCapacity();
	fuel_ = Constants::FuelCapacity();
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
	float turn_rate = (control_flags_ & FORWARD) ? Constants::AccelTurnRate() : Constants::StillTurnRate();

	if (control_flags_ & LEFT)
		direction_.RotateCCW(turn_rate * delta_sec);
	else if (control_flags_ & RIGHT)
		direction_.RotateCW(turn_rate * delta_sec);

	// Acceleration
	Vector2f accel;
	if (control_flags_ & FORWARD)
		accel = direction_.ToVector(50);
	else if (control_flags_ & BACKWARD)
		accel = -direction_.ToVector(50);

	// Velocity
	vel_ += accel * delta_sec;

	// Position
	pos_ += vel_ * delta_sec;
}

void Heli::UpdateWeapons(unsigned int deltams) {
	// take key taps into account
	int combined_control_flags = control_flags_ | tick_control_flags_;

	// Cooldown
	if (gun_reload_ > 0)
		gun_reload_ -= deltams;
	if (hydra_reload_ > 0)
		hydra_reload_ -= deltams;
	if (hellfire_reload_ > 0)
		hellfire_reload_ -= deltams;

	// Process gunfire
	if (combined_control_flags & GUN && guns_ > 0 && gun_reload_ <= 0) {
		float yawdispersion = (2.0 * std::rand() / RAND_MAX - 1.0) * Constants::GunDispersion();
		float pitchdispersion = (2.0 * std::rand() / RAND_MAX - 1.0) * Constants::GunDispersion();
		game_.Spawn<Projectile>(
				pos_ + Constants::GunOffset() * GetSectorDirection(),
				vel_,
				Direction3f(GetSectorDirection().yaw + yawdispersion, Constants::WeaponFirePitch() + pitchdispersion),
				Projectile::BULLET
			);

		guns_--;
		gun_reload_ = Constants::GunCooldown();
	}

	if (combined_control_flags & HYDRA && hydras_ > 0 && hydra_reload_ <= 0) {
		Vector3f mount_offset = Constants::RocketMountOffset();
		if (hydra_at_left_)
			mount_offset.x = -mount_offset.x;

		game_.Spawn<Projectile>(
				pos_ + mount_offset * GetSectorDirection(),
				vel_,
				Direction3f(GetSectorDirection(), Constants::WeaponFirePitch()),
				Projectile::HYDRA
			);

		hydras_--;
		hydra_reload_ = Constants::HydraCooldown();
		hydra_at_left_ = !hydra_at_left_;
	}

	if (tick_control_flags_ /* hellfires have no autofire */ & HELLFIRE && hellfires_ > 0 && hellfire_reload_ <= 0) {
		Vector3f mount_offset = Constants::RocketMountOffset();
		if (hellfire_at_left_)
			mount_offset.x = -mount_offset.x;

		game_.Spawn<Projectile>(
				pos_ + mount_offset * GetSectorDirection(),
				vel_,
				Direction3f(GetSectorDirection(), Constants::WeaponFirePitch()),
				Projectile::HELLFIRE
			);

		hellfires_--;
		hellfire_reload_ = Constants::HellfireCooldown();
		hellfire_at_left_ = !hellfire_at_left_;
	}

	// XXX: no-ammo case should emit clicking sound; this
	// probably should be done via emitting some kind of event
}
