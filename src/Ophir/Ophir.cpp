#include "Ophir/Ophir.h"
#include "Diagnostics/Diagnostics.h"


Ophir::~Ophir(){
    //OphirLM.CloseAll(); //close device
    std::cout << "Ophir destructed" << std::endl;
};

void Ophir::connect(){
    if(this->init){
        this->disconnect();
    }
    
    try{
        
        std::vector<std::wstring> serialNumbers;
        OphirLM.ScanUSB(serialNumbers);

        if(serialNumbers.size() == 0){
            std::cout << "Ophir not found" << std::endl;
            return;
        }

        OphirLM.OpenUSBDevice(Ophir::serial.c_str(), this->hDevice);

        std::wstring info ,headSN, headType, headName, version;
        std::wstring deviceName, romVersion, serialNumber;
        OphirLM.GetDeviceInfo(this->hDevice, deviceName, romVersion, serialNumber);
        OphirLM.GetSensorInfo(this->hDevice, Ophir::channel, headSN, headType, headName); 
        //std::wcout << L"Head name:          " << headName << L" \n"; // PE50-DIF-C
        //std::wcout << L"Head sn:          " << headSN << L" \n";    // 959905
        
        
        std::cout << "OPHIR SETTINGS!!!\n\n" << std::endl;
        
        this->init = true;
        this->arm();
	}catch (const _com_error& e){
		std ::wcout << L"Error 0x" << std::hex << e.Error() << L" " << e.Description()  << L"\n";
		// 0x00000000 (S_OK) : No Error
		// 0x80070057 (E_INVALIDARG) : Invalid Argument
		// 0x80040201 : Device Already Opened
		// 0x80040300 : Device Failed
		// 0x80040301 : Device Firmware is Incorrect
		// 0x80040302 : Sensor Failed
		// 0x80040303 : Sensor Firmware is Incorrect
		// 0x80040306 : This Sensor is Not Supported
		// 0x80040308 : The Device is no longer Available
		// 0x80040405 : Command Failed
		// 0x80040501 : A channel is in Stream Mode
	}
};

Json Ophir::handleRequest(Json& req){
    if(req.contains("reqtype")){
        if(req.at("reqtype") == "status"){
            return this->status();
        }else if(req.at("reqtype") == "arm"){
            this->arm();
            return this->status();
        }else if(req.at("reqtype") == "disarm"){
            this->disarm();
            return this->status();
        }else{
            return {
                    {"ok", false},
                    {"err", "reqtype not found"}
            };
        }
    }else{
        return {
                {"ok", false},
                {"err", "request has no 'reqtype'"}
        };
    }
};

Json Ophir::status(){   
    return Json({
        {"ok", this->init},
        {"armed", this->armed.load()},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()},
        {"curr", 0}
    });
};

void Ophir::arm(){
    if(this->init && !this->armed){
        std::cout << "ophir arming" << std::endl;
        
        this->values.clear();
        this->timestamps.clear();
        this->statuses.clear();

        
        try{
        
            //this->values.reserve();
            
            std::cout << "config ..." << std::endl;
            //OphirLM.ConfigureStreamMode(this->hDevice, this->channel, 2, 1); // disable buffering
    
            OphirLM.StartStream(this->hDevice, this->channel);
            std::cout << "config OK" << std::endl;
            this->armed = true;
    
                
        }catch (const _com_error& e){
            std ::wcout << L"Error 0x" << std::hex << e.Error() << L" " << e.Description()  << L"\n";
        }


        bool stopped = false;

        long long begin = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        while(!(stopped)){
            //stopped = true;
    
            try{
                //these are buffers.
                std::vector<double> values; //Joul
                std::vector<double> timestamps; //ms
                std::vector<OphirLMMeasurement::Status> statuses; //0 = ok, 4=reset
        
                OphirLM.GetData(this->hDevice, this->channel, values, timestamps, statuses);
                
                for (size_t i = 0; i < values.size(); ++i){
                    std::wcout << L"Timestamp: " << std::fixed << std::setprecision(3) << timestamps[i]
                        << L" Reading: " << std::scientific << values[i] << L" Status: " << statuses[i]<< L"\n"; 
                }
                if(values.size() > 0){
                    std::cout << "Ophir got " << values.size() << std::endl;
                }
                
                //When inserting a range, the range version of insert() is generally preferable as it preserves the correct capacity growth behavior, unlike reserve() followed by a series of push_back()s.
                //extend real storage
                
                //check stop condition
                //add current
                if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - begin > 10*1000){
                    stopped = true;

                }
            }catch (const _com_error& e){
                std ::wcout << L"Error 0x" << std::hex << e.Error() << L" " << e.Description()  << L"\n";
            }
        }
        armed = false;
        std::cout << "ophir worker done" << std::endl;
        return;
        
    }
    
};

void Ophir::disarm(){
    if(this->init){
        this->armed = false;
        try{
            this->worker.request_stop();
            OphirLM.StopAllStreams(); //stop measuring
            OphirLM.CloseAll();
        }catch (const _com_error& e){
            std ::wcout << L"Error 0x" << std::hex << e.Error() << L" " << e.Description()  << L"\n";
        }
    }
};

void Ophir::disconnect(){
    this->disarm();
    this->init = false;
    try{
        std::wcout << L"Device has been removed from the USB. \n";
        OphirLM.CloseAll(); //close device
    }catch (const _com_error& e){
        std ::wcout << L"Error 0x" << std::hex << e.Error() << L" " << e.Description()  << L"\n";
    }
};