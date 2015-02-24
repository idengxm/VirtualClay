#pragma once
#include "Leap_Fingers.h"
namespace mb = mudbox;
class Leap_Hand
{
  LR lr;
  std::vector<Leap_Fingers *> fingers;
  std::vector<std::vector<Leap_Fingers *>> fings;
  Leap_Fingers *palm;
  mb::Vector handRot;
  mb::Vector handPos;
  mb::Node *Node;
  mb::Transformation *TNode;
  mb::GroupNode *GNode;
  mb::Geometry *GeoNode;
  mb::Mesh *MNode;
  ID_List *idList;
public:
  Leap_Hand(ID_List *idl,LR lOrR);
  ~Leap_Hand(void);
  void AddLeap_Fingers(void);
  void AddHand(void);
  void SetRot(mb::Vector v);
  void SetPos(mb::Vector v);
  mb::Vector GetPos();
  void SetVisi(bool vis);
  void SetFingerPos(fingerEnum fn,mb::Vector v);
  void SetFingerPos(jointEnum j, fingerEnum f, mb::Vector v);
  void SetFingerRot(fingerEnum fn,mb::Vector v);
  void Leap_Hand::RotateAroundPivot(jointEnum j,fingerEnum f,mb::Vector a,mb::Vector pivot);
  void RotateAroundPivot(fingerEnum fn,mb::Vector a,mb::Vector pivot);
  void RotateAroundPivot(mb::Vector a,mb::Vector pivot);
  mb::AxisAlignedBoundingBox GetFingerBoundingBox(fingerEnum f, jointEnum j = jointEnum(0));
  mb::Vector Leap_Hand::GetFingerPos(fingerEnum f, jointEnum j = jointEnum(0));
  mb::Vector Leap_Hand::GetRot();
/*
  mb::Vector GetFingerPos(fingerEnum fn);*/
//TODO: 
//Set IndexFinger to Thumb Rot and pos

};

