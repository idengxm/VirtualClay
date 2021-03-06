#pragma once
#include "stdafx.h"
#include "Leap_Fingers.h"
#include "Leap_Hand.h"
#include "AirPen_Server.h"

class Leap_Tool
{
  std::vector<Leap_Fingers *> tools;
  mb::Image *stamp;
  QImage *img;
  QImage *OriginalImg;
  AirPen_Server *server;
public:
  //Constructor
  Leap_Tool(void);
  Leap_Tool(Leap_Hand *copyNode);
  ~Leap_Tool(void);
  
  //Stamp
  void SetStamp(QString fullPath);
  void ReleaseStamp();
  mb::Image* GetStamp();
  void ResizeStamp(float x, float y);

  //Tool GUI
  void SetVisi(bool vis);
  void SetPos(int i, mb::Vector &pos);
  void SetPos(std::vector<mb::Vector> &pos);
  void SetRot(int i, mb::Vector &pos);

  void SetRot(mb::Vector &rotation);
  void RotateAroundPivot(int i, mb::Vector &a, mb::Vector &pivot);
  void RotateAroundPivot(mb::Vector &a, mb::Vector &pivot);
  mb::Vector GetPos(int i);
  mb::AxisAlignedBoundingBox GetBoundingBox(int i);
  mb::AxisAlignedBoundingBox GetInteractionBox();
  void SendToServer(int stage);
  float GetStampStrength(mb::Vector &uv);
};

