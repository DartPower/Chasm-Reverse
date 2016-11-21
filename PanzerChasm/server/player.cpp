#include <cstring>

#include "../map_loader.hpp"
#include "./math_utils.hpp"

#include "player.hpp"

namespace PanzerChasm
{

Player::Player( const GameResourcesConstPtr& game_resources )
	: game_resources_( game_resources )
	, pos_( 0.0f, 0.0f, 0.0f )
	, speed_( 0.0f, 0.0f, 0.0f )
	, on_floor_(false)
	, noclip_(false)
	, health_(100)
	, armor_(0)
{
	PC_ASSERT( game_resources_ != nullptr );

	for( unsigned int i= 0u; i < GameConstants::weapon_count; i++ )
	{
		ammo_[i]= 0u;
		have_weapon_[i]= false;
	}

	have_weapon_[0]= true;
}

Player::~Player()
{}

void Player::SetPosition( const m_Vec3& pos )
{
	pos_= pos;
}

void Player::ClampSpeed( const m_Vec3& clamp_surface_normal )
{
	const float projection= clamp_surface_normal * speed_;
	if( projection < 0.0f )
		speed_-= clamp_surface_normal * projection;
}

void Player::SetOnFloor( bool on_floor )
{
	on_floor_= on_floor;
}

bool Player::TryActivateProcedure( const unsigned int proc_number, const Time current_time )
{
	PC_ASSERT( proc_number != 0u );

	if( proc_number == last_activated_procedure_ )
	{
		// Activate again only after delay.
		if( current_time - last_activated_procedure_activation_time_ <= Time::FromSeconds(2) )
			return false;
	}

	last_activated_procedure_= proc_number;
	last_activated_procedure_activation_time_= current_time;
	return true;
}

void Player::ResetActivatedProcedure()
{
	last_activated_procedure_= 0u;
	last_activated_procedure_activation_time_= Time::FromSeconds(0);
}

bool Player::TryPickupItem( const unsigned int item_id )
{
	if( item_id >=  game_resources_->items_description.size() )
		return false;

	const ACode a_code= static_cast<ACode>( game_resources_->items_description[ item_id ].a_code );

	if( a_code >= ACode::Weapon_First && a_code <= ACode::Weapon_Last )
	{
		const unsigned int weapon_index=
			static_cast<unsigned int>( a_code ) - static_cast<unsigned int>( ACode::Weapon_First );

		const GameResources::WeaponDescription& weapon_description= game_resources_->weapons_description[ weapon_index ];

		if( have_weapon_[ weapon_index ] &&
			ammo_[ weapon_index ] >= static_cast<unsigned int>( weapon_description.limit ) )
			return false;

		have_weapon_[ weapon_index ]= true;

		unsigned int new_ammo_count= ammo_[ weapon_index ] + static_cast<unsigned int>( weapon_description.start );
		new_ammo_count= std::min( new_ammo_count, static_cast<unsigned int>( weapon_description.limit ) );
		ammo_[ weapon_index ]= new_ammo_count;

		return true;
	}
	else if( a_code >= ACode::Ammo_First && a_code <= ACode::Ammo_Last )
	{
		const unsigned int weapon_index=
			static_cast<unsigned int>( a_code ) / 10u - static_cast<unsigned int>( ACode::Weapon_First );

		const unsigned int ammo_portion_size= static_cast<unsigned int>( a_code ) % 10u;

		const GameResources::WeaponDescription& weapon_description= game_resources_->weapons_description[ weapon_index ];

		if( ammo_[ weapon_index ] >= static_cast<unsigned int>( weapon_description.limit ) )
			return false;

		unsigned int new_ammo_count=
			ammo_[ weapon_index ] +
			static_cast<unsigned int>( weapon_description.d_am ) * ammo_portion_size;

		new_ammo_count= std::min( new_ammo_count, static_cast<unsigned int>( weapon_description.limit ) );
		ammo_[ weapon_index ]= new_ammo_count;

		return true;
	}
	else if( a_code == ACode::Item_Life )
	{
		if( health_ < 100 )
		{
			health_= std::min( health_ + 20, 100 );
			return true;
		}
		return false;
	}
	else if( a_code == ACode::Item_BigLife )
	{
		if( health_ < 200 )
		{
			health_= std::min( health_ + 100, 200 );
			return true;
		}
		return false;
	}
	else if( a_code == ACode::Item_Armor )
	{
		if( armor_ < 200 )
		{
			armor_= std::min( armor_ + 200, 200 );
			return true;
		}
		return false;
	}
	else if( a_code == ACode::Item_Helmet )
	{
		if( armor_ < 200 )
		{
			armor_= std::min( armor_ + 100, 200 );
			return true;
		}
		return false;
	}

	return false;
}

void Player::BuildStateMessage( Messages::PlayerState& out_state_message ) const
{
	for( unsigned int i= 0u; i < GameConstants::weapon_count; i++ )
		out_state_message.ammo[i]= ammo_[i];

	out_state_message.health= std::max( 0, health_ );
	out_state_message.armor= armor_;

	out_state_message.keys_mask= 0u;
	if( have_red_key_   ) out_state_message.keys_mask|= 1u;
	if( have_green_key_ ) out_state_message.keys_mask|= 2u;
	if( have_blue_key_  ) out_state_message.keys_mask|= 4u;
}

void Player::UpdateMovement( const Messages::PlayerMove& move_message )
{
	mevement_acceleration_= float(move_message.acceleration) / 255.0f;
	movement_direction_= MessageAngleToAngle( move_message.angle );
	jump_pessed_= move_message.jump_pressed;
}

void Player::Move( const Time time_delta )
{
	const float time_delta_s= time_delta.ToSeconds();

	// TODO - calibrate this
	const float c_max_speed= 5.0f;
	const float c_acceleration= 40.0f;
	const float c_deceleration= 20.0f;
	const float c_jump_speed_delta= 3.3f;
	const float c_vertical_acceleration= -9.8f;
	const float c_max_vertical_speed= 5.0f;

	const float speed_delta= time_delta_s * mevement_acceleration_ * c_acceleration;
	const float deceleration_speed_delta= time_delta_s * c_deceleration;

	// Accelerate
	speed_.x+= std::cos( movement_direction_ ) * speed_delta;
	speed_.y+= std::sin( movement_direction_ ) * speed_delta;

	// Decelerate
	const float new_speed_length= speed_.xy().Length();
	if( new_speed_length >= deceleration_speed_delta )
	{
		const float k= ( new_speed_length - deceleration_speed_delta ) / new_speed_length;
		speed_.x*= k;
		speed_.y*= k;
	}
	else
		speed_.x= speed_.y= 0.0f;

	// Clamp speed
	const float new_speed_square_length= speed_.xy().SquareLength();
	if( new_speed_square_length > c_max_speed * c_max_speed )
	{
		const float k= c_max_speed / std::sqrt( new_speed_square_length );
		speed_.x*= k;
		speed_.y*= k;
	}

	// Fall down
	speed_.z+= c_vertical_acceleration * time_delta_s;

	// Jump
	if( jump_pessed_ && noclip_ )
		speed_.z-= 2.0f * c_vertical_acceleration * time_delta_s;
	else if( jump_pessed_ && on_floor_ && speed_.z <= 0.0f )
		speed_.z+= c_jump_speed_delta;

	// Clamp vertical speed
	if( std::abs( speed_.z ) > c_max_vertical_speed )
		speed_.z*= c_max_vertical_speed / std::abs( speed_.z );

	pos_+= speed_ * time_delta_s;

	if( noclip_ && pos_.z < 0.0f )
	{
		pos_.z= 0.0f;
		speed_.z= 0.0f;
	}
}

void Player::SetNoclip( const bool noclip )
{
	noclip_= noclip;
}

bool Player::IsNoclip() const
{
	return noclip_;
}

void Player::GiveRedKey()
{
	have_red_key_= true;
}

void Player::GiveGreenKey()
{
	have_green_key_= true;
}

void Player::GiveBlueKey()
{
	have_blue_key_= true;
}

void Player::GiveAllKeys()
{
	GiveRedKey();
	GiveGreenKey();
	GiveBlueKey();
}

bool Player::HaveRedKey() const
{
	return have_red_key_;
}

bool Player::HaveGreenKey() const
{
	return have_green_key_;
}

bool Player::HaveBlueKey() const
{
	return have_blue_key_;
}

m_Vec3 Player::Position() const
{
	return pos_;
}

} // namespace PanzerChasm
