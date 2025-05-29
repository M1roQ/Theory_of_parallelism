import cv2
import threading
import time
import argparse
from queue import LifoQueue,Queue
import logging
from typing import List
import os

class Sensor:
    def get(self):
        raise NotImplementedError("subclasses must implement method get()")



class SensorX(Sensor):
    """Sensor X"""
    def __init__(self, delay: float):
        self.delay = delay
        self.data = 0

    def get(self) -> int:
        time.sleep(self.delay)
        self.data += 1
        return self.data


class SensorCam(Sensor):
    def __init__(self,camera_index:int = 0 ,width:int = 640,height:int = 480):
        self.error_logged = False

        global bigerror
        try:
            self._cap = cv2.VideoCapture(camera_index)
            if not self._cap.isOpened():
                if not self.error_logged:
                    logging.error("No camera found with this index")
                    print("No camera found with this index! The error has been logged.")
                    bigerror = True
                    self.error_logged = True
                self.stop()
            else:
                self.error_logged = False
                
            self._cap.set(cv2.CAP_PROP_FRAME_WIDTH,width)
            self._cap.set(cv2.CAP_PROP_FRAME_HEIGHT,height)
            real_width = int(self._cap.get(cv2.CAP_PROP_FRAME_WIDTH))
            real_height = int(self._cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
            print(f"Camera resolution: {real_width}x{real_height}")

        except Exception as e:
            if not self.error_logged:
                logging.error("We have an error : %s",str(e))
                print(f"We have an error : %s, {str(e)}. The error has been logged.")
                bigerror = True
                self.error_logged = True
            self.stop()

    
    def get(self):
        ret,frame = self._cap.read()
        if not ret:
            if not self.error_logged:
                logging.error("No image from camera")
                print("No image from camera. The error has been logged.")

                global bigerror
                bigerror = True
                self.error_logged = True
                self.stop()
            return None
        else:
            self.error_logged = False
            return frame

    def stop(self):
        self._cap.release()
        

    def __del__(self):
        self.stop()

def sensor_worker(sensor:SensorX,queue:LifoQueue):
    global flag
    
    while flag:
        a = sensor.get()
        
        if queue.full():
            queue.get()
            queue.put(a)
        else:
            queue.put(a)
        
    print(f"Sensor с delay = {sensor.delay} - Готово")

    

def camera_worker(queue: Queue, camera_index: int, width: int, height: int):
    global flag
    cam = SensorCam(camera_index=camera_index, width=width, height=height)
    while flag:
        a = cam.get()
        if queue.full():
            queue.get()
            queue.put(a)
        else:
            queue.put(a)
    cam.stop()


class ImageWindow():
    def __init__(self,fps:int = 15,height:int = 480):
        self._sensor_data = [0,0,0]
        self.frame = None
        self.fps = fps
        self._height = height
        self.error_logged = False
    def show(self,cam_queue:Queue,queues:List[Queue]):
        try:
            
            for i in range(3):
                if queues[i].full():
                    self._sensor_data[i] = queues[i].get()
            if cam_queue.full():
                self.frame=cam_queue.get()
            
            if self.frame is not None:
                cv2.putText(self.frame,f"Sensor1: {self._sensor_data[0]}  Sensor2: {self._sensor_data[1]}  Sensor3: {self._sensor_data[2]}", (10,self._height-10), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 255), 1)
                cv2.imshow('camera and data',self.frame)
                self.error_logged = False
            else:
                raise ValueError("Frame is None")
        except Exception as e:
            if not self.error_logged:
                logging.error("We have an error at show(): %s",str(e))
                print(f"We have an error : %s, {str(e)}. The error has been logged.")
                self.error_logged = True

    def stop(self):
        cv2.destroyAllWindows()

    def __del__(self):
        self.stop()



if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Ширина, высота, FPS")
    parser.add_argument("--camIndex",type=int,default=0)
    parser.add_argument("--height", type=int, default =480 )
    parser.add_argument("--width", type=int, default = 720)
    parser.add_argument("--fps", type=int, default=15)
    args = parser.parse_args()
    if not os.path.exists('log'):
        os.makedirs('log')
    
    log_file = os.path.join('log','error.log')
    logging.basicConfig(filename=log_file, level=logging.ERROR,
                        format='%(asctime)s - %(levelname)s - %(message)s')

    flag = True
    bigerror = False
    sensors = [SensorX(i) for i in [0.01,0.1,1]]
    sensor_queues = [LifoQueue(maxsize=1) for _ in range(3)]
    cam_queue = Queue(maxsize=1)
    sensor_workers = [threading.Thread(target=sensor_worker,args=(sensors[i],sensor_queues[i])) for i in range(3)]
    cam_worker = threading.Thread(target=camera_worker, args=(cam_queue, args.camIndex, args.width, args.height))
    time.sleep(1)
    window_imager = ImageWindow(fps = args.fps,height=args.height)
    for i in range(3):
        sensor_workers[i].start()
    cam_worker.start()
    while True:
        
        window_imager.show(cam_queue,sensor_queues)
        if cv2.waitKey(1) & 0xFF == ord('q') or bigerror:
            window_imager.stop()
            
            flag=False
            cam_worker.join()
            for sensor_workerr in sensor_workers:
                sensor_workerr.join()
            break
        
        time.sleep(1/window_imager.fps)