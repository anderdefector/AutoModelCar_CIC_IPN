//Version final 27/06/17 
#include <ros/ros.h>
#include <opencv2/opencv.hpp>
#include <image_transport/image_transport.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <cv_bridge/cv_bridge.h>  
#include <sensor_msgs/image_encodings.h>
#include <ros/console.h>
#include <vector>
#include <iostream>
#include "std_msgs/Int16.h"
cv::Vec4f line;
std_msgs::Int16 angDir,vel;
int antCent=128;
int x,y,cont; 
int MAX_DIST=50;
int MAX_SPEED_BACK=300;
int MAX_SPEED_FOR=-1000;
int MAX_STEER_LEFT=20;
int MAX_STEER_RIGHT=160;
int LANE_WIDTH=4;
int TOL=3;
int err = 0;
int tht = 98;
int errAnt = 0;
int MAX_VEL = -600;

class LineFollow
{
ros::NodeHandle nh_;
image_transport::ImageTransport it_;
image_transport::Subscriber image_sub_;
image_transport::Publisher image_pub_; 
ros::Publisher pubDir,pubVel;

public:
    LineFollow() 
    : it_(nh_)  
  {
     image_sub_ = it_.subscribe("/img_prepros", 1,&LineFollow::imageCb, this);
     image_pub_ = it_.advertise("/lane_follower", 1); 
     pubDir= nh_.advertise<std_msgs::Int16>("/lane_follower/steering",1);//Para publicar en consola
     pubVel= nh_.advertise<std_msgs::Int16>("/lane_follower/speed",1);
    // pubDir= nh_.advertise<std_msgs::Int16>("/manual_control/steering",1); //Para publicar en el carrito
    //pubVel= nh_.advertise<std_msgs::Int16>("/manual_control/speed",1);
     
  }

int dirControl(int err,int tht, int errAnt) 
{
    int pe;
    int pp;
    int sal;    
	  if (tht>110 || tht<90)
    {
		pe = 1.25;
		pp = 1;
		MAX_VEL = -600;
    }
	else	
		{
    pe = 0.3;
		pp = 1;
		MAX_VEL=-700;
    }
	  sal = err*pe + tht*pp + (err-errAnt)*1;
    return sal;
}
void imageCb(const sensor_msgs::ImageConstPtr& msg)
  {
    	cv_bridge::CvImagePtr cv_ptr;
    try
    	{
     	cv_ptr = cv_bridge::toCvCopy(msg,sensor_msgs::image_encodings::MONO8);
    	}
    catch (cv_bridge::Exception& e) 
    	{
      	ROS_ERROR("cv_bridge exception: %s", e.what());
    return;
      }
       int e1=cv::getTickCount();
///////////////////////Color transform///////////////////////////////
       cv::Mat image=cv_ptr->image;
       int hght=image.size().height;
       int wdth=image.size().width;
       //Binarización
       cv::Mat bin;
       cv:inRange(image,140,190,bin);
/////////////////////////Lane center detection//////////////////////////
       int jI=antCent;
       int jD=antCent;
       int point=antCent;
       int i=hght-10;
       int desv;  
       
       std::vector<cv::Point> cents;
       std::vector<cv::Point> iniIzq;
       std::vector<cv::Point> iniDer;
       std::vector<int> pc;
   
       cents.push_back(cv::Point2f(antCent,i));
       iniIzq.push_back(cv::Point2f(antCent-MAX_DIST,i));
       iniDer.push_back(cv::Point2f(antCent+MAX_DIST,i));  
        
       cv::circle(image,cv::Point(cents.back()),3,155,-1);
       cv::circle(image,cv::Point(iniIzq.back()),3,255,-1);
       cv::circle(image,cv::Point(iniDer.back()),3,55,-1); 
       
while (i>hght*0.8)
  { 
   cont=0;  

 //Lado Derecho
   bool centDerFlag = false;
   while ((cents.back().x <= jD) && (jD < wdth))
     	{     
          if(bin.at<uchar>(i,jD)!=0)
              {
                 cont++;
                 pc.push_back(jD); 
               }
          else
               {
                 if( LANE_WIDTH-TOL < cont && cont< LANE_WIDTH+TOL )
                     {
                        x=std::abs(iniDer.back().x-pc[0]);
                        y=std::abs(iniDer.back().y-i);
                          
	                if (x<(20*TOL) && y<(10*TOL))
                             { 
                               iniDer.push_back(cv::Point2f(pc[0],i));
                               cv::circle(image,cv::Point(iniDer.back()),1,100,-1);
                               point= iniDer.back().x-TOL;
                               centDerFlag = true;
                               jD=wdth;
                             }
                        else 
                             {  

  		  		point=cents.back().x;
                             }
                      }  
                 else 
                      {
                          point=cents.back().x;
                      }
                    cont = 0 ;
                    pc.clear();      
              }
         jD++;   
         }
 jD=point;   
//Lado izquierdo
 bool centIzqFlag = false;
 while((wdth > cents.back().x) && (cents.back().x >= jI) && (jI > 0))
    {
 	if (bin.at<uchar>(i,jI)!=0)
        {
 		cont++;
                pc.push_back(jI); 
        }
        else 
        {
            if ((LANE_WIDTH-TOL<cont) && (cont<LANE_WIDTH+TOL))
             {
                x=std::abs(iniIzq.back().x-pc[0]);
                y=std::abs(iniIzq.back().y-i);
               if (x<(20*TOL) && y<(25*TOL))
                   {
                    iniIzq.push_back(cv::Point2f(pc[0],i));
                    cv::circle(image,cv::Point(iniIzq.back()),1,100,-1);
                    point= iniIzq.back().x+(3*TOL);
                    centIzqFlag = true;
                    jI=0;
                   }
               else 
                   {
                    point=cents.back().x;  
                   }
             }
            else
             {
                point=cents.back().x;   
             }
          cont=0;
          pc.clear();   
          }
     jI--; 
     }
jI=point;
if (centDerFlag == true)
  {
   int cent= iniDer.back().x-MAX_DIST;
   cents.push_back(cv::Point2f(cent,i));
   antCent= cents[1].x;
  }

else if (centIzqFlag == true)
  {
   int cent= iniIzq.back().x+MAX_DIST;
   cents.push_back(cv::Point2f(cent,i));
   antCent= cents[1].x;          
  }
cv::circle(image,cv::Point(cents.back()),1,155,-1);		
i=i-4;	
}
///////////////////////////////Steering calculation///////////////////////////////////
bool rf=false;
bool DerFlag=false;
bool IzqFlag=false;
if (iniDer.size()>8)
{
     cv::fitLine(iniDer,line,CV_DIST_L2,0,0.01,0.01);
     desv =-MAX_DIST;
     rf=true;    
     DerFlag=true;    
}		
else if (iniIzq.size()>5)
{
     cv::fitLine(iniIzq,line,CV_DIST_L2,0,0.01,0.01);;
     desv = MAX_DIST;
     rf=true; 
     IzqFlag=true;
}		

if (rf==true)
{
       err = wdth/2-cents[1].x;	
       int p1=line[2];
       int p2=line[3];
       int p3=(line[2]+1)+100*line[0];
       int p4=(line[3]+1)+100*line[1];
       if (IzqFlag==true)
       {
         p3=iniIzq.back().x;
         p4=iniIzq.back().y; 
       }
       else if (DerFlag==true)
       {
        p3=iniDer.back().x;
        p4=iniDer.back().y; 
       }

       float m1=p4-p2;
       float m2=p3-p1;
       float m=(m2/m1); 
       int thtCalc = 98 + int(57*atan(m));
       cv::circle(image,cv::Point(p1,p2),6,200,-1);
       cv::circle(image,cv::Point(p3,p4),6,200,-1);
       cv::line(image,cv::Point(p1,p2),cv::Point(p3,p4),100,3);
       tht = thtCalc;                                     
       ROS_INFO(" tht= %i",tht);
       
}
ROS_INFO(" error= %i",err);
errAnt = err;
int calcDir = dirControl(err, tht, errAnt); 
ROS_INFO(" Calcdir= %i ",calcDir); 
if (calcDir>angDir.data+3)
{
angDir.data += 3;
}
else if (calcDir<angDir.data-3)
{
angDir.data -= 3;
}
////////////////////////////////Speed Calculation and publication///////////////////////////////////////////
int maxVel = int(MAX_VEL*exp(-1*(0.002)*std::abs(err*10)));
ROS_INFO(" MaxVel= %i ",maxVel); 
if (vel.data > (maxVel+9))
   {vel.data -= 2;}
else if (vel.data<=maxVel)
   {vel.data += 5;}
/////////////////////////////////Servomotor and saturation////////////////////////////////////////////////
if (angDir.data>MAX_STEER_RIGHT)
{
    angDir.data = MAX_STEER_RIGHT;
}
else if (angDir.data<MAX_STEER_LEFT)
{
    angDir.data = MAX_STEER_LEFT;
}
pubDir.publish(angDir);
pubVel.publish(vel);
cv_bridge::CvImage out_msg;
out_msg.header = msg->header;
out_msg.encoding= sensor_msgs::image_encodings::MONO8; //BGR8
out_msg.image= image; 
image_pub_.publish(out_msg.toImageMsg());

int e2=cv::getTickCount();
float t=(e2-e1)/cv::getTickFrequency();
ROS_INFO("frame time: %f-------------------------------block end",t);        

  }
};
int main(int argc, char** argv) 
{
ros::init(argc, argv, "lane_follower");
ROS_INFO("my_node running...");
LineFollow ic;
ros::spin();
return 0;
}
