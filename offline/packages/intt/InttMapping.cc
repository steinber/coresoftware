#include "InttMapping.h"
#include "InttFelixMap.h"

const std::map<int, int> Intt::Packet_Id =
{
	{3001, 0},
	{3002, 1},
	{3003, 2},
	{3004, 3},
	{3005, 4},
	{3006, 5},
	{3007, 6},
	{3008, 7},
};

struct Intt::RawData_s Intt::RawFromPacket(int const _i, int const _n, Packet* _p)
{
	struct RawData_s s;

	if(!_p)return s;

	s.felix_server = _i;
	s.felix_channel = _p->iValue(_n, "FEE");
	s.chip = (_p->iValue(_n, "CHIP_ID") + 25) % 26;
	s.channel = _p->iValue(_n, "CHANNEL_ID");

	return s;
}

struct Intt::Online_s Intt::ToOnline(struct Offline_s const& _s)
{
	struct Online_s s;
	int n_ldr = _s.layer < 5 ? 12 : 16;

	s.lyr = _s.layer - 3;
	s.ldr = (7 * n_ldr / 4 - _s.ladder_phi + (_s.layer % 2 ? n_ldr - 1 : 0)) % n_ldr;

	s.arm = _s.ladder_z / 2;
	switch(_s.ladder_z)
	{
		case 1:
		s.chp = _s.strip_y + 13 * !(_s.strip_x < 128);
		break;

		case 0:
		s.chp = _s.strip_y + 13 * !(_s.strip_x < 128) + 5;
		break;

		case 2:
		s.chp = 12 - _s.strip_y + 13 * (_s.strip_x < 128);
		break;

		case 3:
		s.chp = 4 - _s.strip_y + 13 * (_s.strip_x < 128);
		break;

		default:
		break;
	}

	s.chn = (_s.strip_x < 128) ? _s.strip_x : 255 - _s.strip_x;

	return s;
}

struct Intt::Offline_s Intt::ToOffline(struct Online_s const& _s)
{
	struct Offline_s s;
	int n_ldr = _s.lyr < 2 ? 12 : 16;

	s.layer = _s.lyr + 3;
	s.ladder_phi = (7 * n_ldr / 4 - _s.ldr + (_s.lyr % 2 ? 0 : n_ldr - 1)) % n_ldr;

	s.ladder_z = 2 * _s.arm + (_s.chp % 13 < 5);
	switch(s.ladder_z)
	{
		case 1:
		s.strip_y = _s.chp % 13;
		break;

		case 0:
		s.strip_y = _s.chp % 13 - 5;
		break;

		case 2:
		s.strip_y = 12 - (_s.chp % 13);
		break;

		case 3:
		s.strip_y = 4 - (_s.chp % 13);
		break;

		default:
		break;
	}

	s.strip_x = (_s.arm == (_s.chp / 13)) ? _s.chn : 255 - _s.chn;

	return s;
}

struct Intt::RawData_s Intt::ToRawData(struct Online_s const& _s)
{
	struct RawData_s s;

	InttFelix::OnlineToRawData(_s, s);
	s.chip = _s.chp;
	s.channel = _s.chn;

	return s;
}

struct Intt::Online_s Intt::ToOnline(struct RawData_s const& _s)
{
	struct Online_s s;

	InttFelix::RawDataToOnline(_s, s);
	s.chp = _s.chip;
	s.chn = _s.channel;

	return s;
}

struct Intt::RawData_s Intt::ToRawData(struct Offline_s const& _s)
{
	return ToRawData(ToOnline(_s));
}

struct Intt::Offline_s Intt::ToOffline(struct RawData_s const& _s)
{
	return ToOffline(ToOnline(_s));
}

bool operator==(struct Intt::RawData_s const& lhs, struct Intt::RawData_s const& rhs)
{
	if(lhs.felix_server != rhs.felix_server)return false;
	if(lhs.felix_channel != rhs.felix_channel)return false;
	if(lhs.chip != rhs.chip)return false;
	if(lhs.channel != rhs.channel)return false;

	return true;
}

bool operator==(struct Intt::Online_s const& lhs, struct Intt::Online_s const& rhs)
{
	if(lhs.lyr != rhs.lyr)return false;
	if(lhs.ldr != rhs.ldr)return false;
	if(lhs.arm != rhs.arm)return false;
	if(lhs.chp != rhs.chp)return false;
	if(lhs.chn != rhs.chn)return false;

	return true;
}

bool operator==(struct Intt::Offline_s const& lhs, struct Intt::Offline_s const& rhs)
{
	if(lhs.layer != rhs.layer)return false;
	if(lhs.ladder_phi != rhs.ladder_phi)return false;
	if(lhs.ladder_z != rhs.ladder_z)return false;
	if(lhs.strip_x != rhs.strip_x)return false;
	if(lhs.strip_y != rhs.strip_y)return false;

	return true;
}

bool operator!=(struct Intt::RawData_s const& lhs, struct Intt::RawData_s const& rhs)
{
	return !(lhs == rhs);
}

bool operator!=(struct Intt::Online_s const& lhs, struct Intt::Online_s const& rhs)
{
	return !(lhs == rhs);
}

bool operator!=(struct Intt::Offline_s const& lhs, struct Intt::Offline_s const& rhs)
{
	return !(lhs == rhs);
}

bool operator<(struct Intt::RawData_s const& lhs, struct Intt::RawData_s const& rhs)
{
	if(lhs.felix_server != rhs.felix_server)return lhs.felix_server < rhs.felix_server;
	if(lhs.felix_channel != rhs.felix_channel)return lhs.felix_channel < rhs.felix_channel;
	if(lhs.chip != rhs.chip)return lhs.chip < rhs.chip;
	if(lhs.channel != rhs.channel)return lhs.channel < rhs.channel;

	return false;
}

bool operator<(struct Intt::Online_s const& lhs, struct Intt::Online_s const& rhs)
{
	if(lhs.lyr != rhs.lyr)return lhs.lyr < rhs.lyr;
	if(lhs.ldr != rhs.ldr)return lhs.ldr < rhs.ldr;
	if(lhs.arm != rhs.arm)return lhs.arm < rhs.arm;
	if(lhs.chp != rhs.chp)return lhs.chp < rhs.chp;
	if(lhs.chn != rhs.chn)return lhs.chn < rhs.chn;

	return false;
}

bool operator<(struct Intt::Offline_s const& lhs, struct Intt::Offline_s const& rhs)
{
	if(lhs.layer != rhs.layer)return lhs.layer < rhs.layer;
	if(lhs.ladder_phi != rhs.ladder_phi)return lhs.ladder_phi < rhs.ladder_phi;
	if(lhs.ladder_z != rhs.ladder_z)return lhs.ladder_z < rhs.ladder_z;
	if(lhs.strip_x != rhs.strip_x)return lhs.strip_x < rhs.strip_x;
	if(lhs.strip_y != rhs.strip_y)return lhs.strip_y < rhs.strip_y;

	return false;
}

bool operator>(struct Intt::RawData_s const& lhs, struct Intt::RawData_s const& rhs)
{
	return rhs < lhs;
}

bool operator>(struct Intt::Online_s const& lhs, struct Intt::Online_s const& rhs)
{
	return rhs < lhs;
}

bool operator>(struct Intt::Offline_s const& lhs, struct Intt::Offline_s const& rhs)
{
	return rhs < lhs;
}

bool operator>=(struct Intt::RawData_s const& lhs, struct Intt::RawData_s const& rhs)
{
	return !(lhs < rhs);
}

bool operator>=(struct Intt::Online_s const& lhs, struct Intt::Online_s const& rhs)
{
	return !(lhs < rhs);
}

bool operator>=(struct Intt::Offline_s const& lhs, struct Intt::Offline_s const& rhs)
{
	return !(lhs < rhs);
}

bool operator<=(struct Intt::RawData_s const& lhs, struct Intt::RawData_s const& rhs)
{
	return !(lhs > rhs);
}

bool operator<=(struct Intt::Online_s const& lhs, struct Intt::Online_s const& rhs)
{
	return !(lhs > rhs);
}

bool operator<=(struct Intt::Offline_s const& lhs, struct Intt::Offline_s const& rhs)
{
	return !(lhs > rhs);
}
