#pragma once

enum PacketType {
    LOCATION_REQUEST = 1,
    INFO_REQUEST = 2,
    INFO_RESPONSE = 3,
    LOCATION_RESPONSE = 4
};

struct Point
{
    double x;
    double y;
    
    Point(double _x, double _y)
        : x(_x), y(_y) {}

    Point () {}
};

struct PacketData {
    int packetType;
    double xCoord;
    double yCoord;
    double batteryLevel;   

    PacketData () {}  

    PacketData (int _packetType, double _xCoord, double _yCoord, double _batteryLevel) : 
        packetType(_packetType), xCoord(_xCoord), yCoord(_yCoord), batteryLevel(_batteryLevel) {}              
};
