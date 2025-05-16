#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <MQTTClient.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <stdarg.h>
#include <cmath>
#include <inttypes.h>

#define HOSTNAME "tcp://192.168.67.70:1883";
#define CLIENT_ID "Flurry";
#define TOPIC "home/coils/ice";
#define TIMEOUT 10000

std::string string_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int size = vsnprintf(nullptr, 0, format, args);
    va_end(args);
    if (size < 0) {
        return "";
    }
    char buffer[size + 1];
    va_start(args, format);
    vsnprintf(buffer, size + 1, format, args);
    va_end(args);
    return std::string(buffer, size);
}

float processFrame( cv::Mat &frame )
{ int rows        = frame.rows;
  int cols        = frame.cols;
  int    iceCount = 0;
  int notIceCount = 1;
  int   threshold = 500;
  int      minCol = (cols * 230) / 1000;
  int      maxCol = (cols * 690) / 1000;
  int      maxRow = (rows * 850) / 1000;
  for ( int j = 0; j < rows; j++ )
    for ( int i = 0; i < cols; i++ )
      { if (( i < minCol ) || ( i > maxCol ) || ( j > maxRow ))
          frame.at<cv::Vec3b>(j, i) = cv::Vec3b(0, 0, 0);
         else
          { cv::Vec3b pixel = frame.at<cv::Vec3b>(j, i);
            int bv = pixel[0] + pixel[1] + pixel[2];
            if ( bv > threshold )
              { iceCount++;
                frame.at<cv::Vec3b>(j, i) = cv::Vec3b(255, 0, 0);
              }
             else
              notIceCount++;
      }   }
  return 100.0*((float)iceCount)/((float)notIceCount);
}

int64_t curTime()
{ auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  return milliseconds;
}

int main(int argc, char* argv[])
{ bool show = false;
  printf( "argc:%d\n", argc );
  if ( argc > 1 )
    show = true;
  int64_t fa_interval      = 5000;
  int64_t start            = curTime() - fa_interval - 1;
    float last_pub_val     = -2.0;
    float pub_val_diff     = 1.0;
  int64_t max_pub_interval = 59000;
  int64_t last_pub_time    = curTime() - max_pub_interval - 1;
 
  // Replace with your RTSP stream URL
  std::string rtsp_url = "rtsp://admin:ice22view@192.168.67.43:554/cam/realmonitor?channel=1&subtype=1";

  // Open the RTSP stream
  cv::VideoCapture capture(rtsp_url);

  // Check if the stream is opened successfully
  if (!capture.isOpened())
    { std::cerr << "Error opening RTSP stream" << std::endl;
      return -1;
    }

  cv::Mat frame;

  MQTTClient client;
  MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
  int rc;
  const char* hostname = HOSTNAME;
  const char* clientid = CLIENT_ID;
  const char* topic = TOPIC;
  const char* message;
  const int timeout = TIMEOUT;
  std::string message_payload;

  // 1. Initialize MQTT client
  rc = MQTTClient_create(&client, hostname, clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL);
  if (rc != MQTTCLIENT_SUCCESS)
    { fprintf(stderr, "Failed to create MQTT client: %d\n", rc);
      return 1;
    }

  // 2. Configure connection options
  opts.keepAliveInterval = 125; // Keep-alive interval in seconds
  opts.cleansession = 1;        // Start a new session
  opts.username = NULL;         // No username
  opts.password = NULL;         // No password
  opts.will = NULL;             // No last will

  // 3. Connect to the broker
  rc = MQTTClient_connect(client, &opts);
  if (rc != MQTTCLIENT_SUCCESS)
    { fprintf(stderr, "Failed to connect to broker: %d\n", rc);
      MQTTClient_destroy(&client);
      return 1;
    }

  // Loop to read and display frames
  while (true)
    { // Read a frame from the stream
      capture >> frame;
      if (frame.empty())
        { std::cerr << "Warning: read empty frame from RTSP stream" << std::endl;
        }
       else
        { int64_t now = curTime();
          if ( now - start >= fa_interval )
            { start = now;
              float fract = processFrame( frame );
              message_payload = string_printf( "%f\n", fract );
              std::cout << message_payload;

              if (( fabs( fract - last_pub_val ) >= pub_val_diff ) ||
                  ( (now - last_pub_time) >= max_pub_interval ))
                { // 4. Publish a message
                  message = message_payload.c_str();
                  rc = MQTTClient_publish(client, topic, strlen(message), message, 0, 0, NULL);
                  if (rc == MQTTCLIENT_SUCCESS)
                    { printf("published, interval %" PRId64 "\n", now - last_pub_time );
                      last_pub_val  = fract;
                      last_pub_time = now;
                    }
                   else
                    { fprintf(stderr, "Failed to publish message: %d\n", rc);
                      if ( rc == MQTTCLIENT_DISCONNECTED )
                        { rc = MQTTClient_connect(client, &opts);
                          if (rc != MQTTCLIENT_SUCCESS)
                            { fprintf(stderr, "Failed to connect to broker: %d\n", rc);
                              MQTTClient_destroy(&client);
                              rc = MQTTClient_create(&client, hostname, clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL);
                              if (rc != MQTTCLIENT_SUCCESS)
                                { fprintf(stderr, "Failed to create MQTT client: %d\n", rc);
                }   }   }   }   }

              if ( show )
                { cv::imshow("RTSP Stream", frame);
            }   }
          // Exit if 'q' is pressed
          if (cv::waitKey(1) == 'q')
            { rc = 0;
              break;
    }   }   }

  // 6. Disconnect from the broker
  rc = MQTTClient_disconnect(client, timeout);
  if (rc != MQTTCLIENT_SUCCESS)
    fprintf(stderr, "Failed to disconnect from broker: %d\n", rc);

  // 7. Destroy the client
  MQTTClient_destroy(&client);

  // Release the capture and destroy all windows
  capture.release();
  cv::destroyAllWindows();

  return rc;
}
