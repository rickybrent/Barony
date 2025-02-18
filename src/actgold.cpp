/*-------------------------------------------------------------------------------

	BARONY
	File: actgold.cpp
	Desc: behavior function for gold

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "messages.hpp"
#include "sound.hpp"
#include "net.hpp"
#include "entity.hpp"
#include "player.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following function describes an entity behavior. The function
	takes a pointer to the entity that uses it as an argument.

-------------------------------------------------------------------------------*/

void actGoldBag(Entity* my)
{
	int i;

	if ( my->ticks == 1 )
	{
		my->createWorldUITooltip();
	}

	if ( my->flags[INVISIBLE] && my->goldSokoban == 1 )
	{
		if ( multiplayer != CLIENT )
		{
			node_t* node;
			for ( node = map.entities->first; node != nullptr; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( entity->isBoulderSprite() )   // boulder.vox
				{
					return;
				}
			}
			my->flags[INVISIBLE] = false;
			serverUpdateEntityFlag(my, INVISIBLE);
			if ( !strcmp(map.name, "Sokoban") )
			{
				for ( i = 0; i < MAXPLAYERS; ++i )
				{
					steamAchievementClient(i, "BARONY_ACH_PUZZLE_MASTER");
				}
			}
		}
		else
		{
			return;
		}
	}

	my->goldAmbience--;
	if ( my->goldAmbience <= 0 )
	{
		my->goldAmbience = TICKS_PER_SECOND * 30;
		playSoundEntityLocal( my, 149, 16 );
	}

	// pick up gold
	if ( multiplayer != CLIENT )
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if ( (i == 0 && selectedEntity[0] == my) || (client_selected[i] == my) || (splitscreen && selectedEntity[i] == my) )
			{
				if (inrange[i])
				{
					if (players[i] && players[i]->entity)
					{
						playSoundEntity(players[i]->entity, 242 + rand() % 4, 64 );
					}
					stats[i]->GOLD += my->goldAmount;
					if ( i != 0 )
					{
						if ( multiplayer == SERVER )
						{
							// send the client info on the gold it picked up
							strcpy((char*)net_packet->data, "GOLD");
							SDLNet_Write32(stats[i]->GOLD, &net_packet->data[4]);
							net_packet->address.host = net_clients[i - 1].host;
							net_packet->address.port = net_clients[i - 1].port;
							net_packet->len = 8;
							sendPacketSafe(net_sock, -1, net_packet, i - 1);
						}
					}

					// message for item pickup
					if ( my->goldAmount == 1 )
					{
						messagePlayer(i, language[483]);
					}
					else
					{
						messagePlayer(i, language[484], my->goldAmount);
					}

					// remove gold entity
					list_RemoveNode(my->mynode);
					return;
				}
			}
		}
	}
	else
	{
		my->flags[NOUPDATE] = true;
	}
}
