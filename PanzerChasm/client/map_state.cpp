#include "map_state.hpp"

namespace PanzerChasm
{

static const float g_walls_coord_scale= 256.0f;

MapState::MapState( const MapDataConstPtr& map )
	: map_data_(map)
{
	PC_ASSERT( map_data_ != nullptr );

	dynamic_walls_.resize( map_data_->dynamic_walls.size() );

	for( unsigned int w= 0u; w < dynamic_walls_.size(); w++ )
	{
		const MapData::Wall& in_wall= map_data_->dynamic_walls[w];
		DynamicWall& out_wall= dynamic_walls_[w];

		out_wall.xy[0][0]= short( in_wall.vert_pos[0].x * g_walls_coord_scale );
		out_wall.xy[0][1]= short( in_wall.vert_pos[0].y * g_walls_coord_scale );
		out_wall.xy[1][0]= short( in_wall.vert_pos[1].x * g_walls_coord_scale );
		out_wall.xy[1][1]= short( in_wall.vert_pos[1].y * g_walls_coord_scale );
		out_wall.z= 0;
	}
}

MapState::~MapState()
{}

const MapState::DynamicWalls& MapState::GetDynamicWalls() const
{
	return dynamic_walls_;
}

void MapState::ProcessMessage( const Messages::EntityState& message )
{
	// TODO
	PC_UNUSED(message);
}

void MapState::ProcessMessage( const Messages::WallPosition& message )
{
	if( message.wall_index >= dynamic_walls_.size() )
		return; // Bad wall index.

	DynamicWall& wall= dynamic_walls_[ message.wall_index ];

	wall.xy[0][0]= message.vertices_xy[0][0];
	wall.xy[0][1]= message.vertices_xy[0][1];
	wall.xy[1][0]= message.vertices_xy[1][0];
	wall.xy[1][1]= message.vertices_xy[1][1];
	wall.z= message.z;
}

void MapState::ProcessMessage( const Messages::EntityBirth& message )
{
	// TODO
	PC_UNUSED(message);
}

void MapState::ProcessMessage( const Messages::EntityDeath& message )
{
	// TODO
	PC_UNUSED(message);
}

} // namespace PanzerChasm
