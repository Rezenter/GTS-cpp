#include "Ophir/Ophir.h"
#include "Diagnostics/Diagnostics.h"

void PlugAndPlayCallback()
{
	std::wcout << L"Device has been removed from the USB. \n";
}

void DataReadyCallback(long hDevice,long channel)
{
	std::wcout << L"data ready callback \n";
}

Ophir::~Ophir(){
    std::cout << "Ophir destructor" << std::endl;
    this->disarm();
    this->thread.request_stop();
    this->thread.join();
    std::cout << "Ophir destructed" << std::endl;
};

void Ophir::connect(){
    if(this->init.load()){
        this->disarm();
        this->thread.request_stop();
        this->thread.join();
    }
    
    this->thread = std::jthread([th=this](std::stop_token stoken){
        SetThreadAffinityMask(GetCurrentThread(), 1 << 9);

        th->channel = th->diag->storage.config["laser"][0]["ophir"]["channel"];

        struct CoInitializer{
            CoInitializer() { CoInitialize(nullptr); }
            ~CoInitializer() { CoUninitialize(); }
        };
        CoInitializer initializer;// must call for COM initialization and deinitialization

        OphirLMMeasurement OphirLM;
        long hDevice = 0;
        try{
            std::vector<std::wstring> results;

            OphirLM.ScanUSB(results);

            if(results.size() == 0){
                std::cout << "Ophir not found" << std::endl;
            }else{
                bool found = false;
                std::wstring expectedSerial;
                for(auto c: (std::string)th->diag->storage.config["laser"].at(0)["ophir"]["ADCSerial"]){
                    expectedSerial.append(1, c);
                }
                for(auto& ser: results){
                    if(expectedSerial.compare(ser) == 0){
                        std::cout << "Ophir found" << std::endl;
                        found = true;
                        break;
                    }
                }
                if(found){
                    OphirLM.OpenUSBDevice(th->serial.c_str(), hDevice);
        
                    std::wstring info ,headSN, headType, headName, version;
                    std::wstring deviceName, romVersion, serialNumber;
                    OphirLM.GetDeviceInfo(hDevice, deviceName, romVersion, serialNumber);
                    OphirLM.GetSensorInfo(hDevice, th->channel, headSN, headType, headName);
                    expectedSerial.clear();
                    for(auto c: (std::string)th->diag->storage.config["laser"].at(0)["ophir"]["headSerial"]){
                        expectedSerial.append(1, c);
                    }
                    if(expectedSerial.compare(headSN) != 0){
                        std::cout << "Ophir has wrong head serial connected to the selected channel" << std::endl;
                        std::wcout << expectedSerial << " " << headSN << std::endl;
                        return;
                    }
                    //std::wcout << L"Head name:          " << headName << L" \n"; // PE50-DIF-C
                    //std::wcout << L"Head sn:          " << headSN << L" \n";    // 959905
                    

                    /*
                    diffuser: int = 1  # diffuser (0, ('N/A',))
                    measurement_mode: int = 1  # MeasMode (1, ('Power', 'Energy'))
                    pulse_length: int = 0 # PulseLengths (0, ('30uS', '1.0mS'))
                    measurement_range: int = 2  #Ranges (2, ('10.0J', '2.00J', '200mJ', '20.0mJ', '2.00mJ', '200uJ'))
                    wavelength: int = 3  #Wavelengths (3, (' 193', ' 248', ' 532', '1064', '2100', '2940'))
                
                    self.OphirCOM.SetMeasurementMode(self.DeviceHandle, 0, self.measurement_mode)
                    self.OphirCOM.SetPulseLength(self.DeviceHandle, 0, self.pulse_length)
                    self.OphirCOM.SetRange(self.DeviceHandle, 0, self.measurement_range)
                    self.OphirCOM.SetWavelength(self.DeviceHandle, 0, self.wavelength)
                    */
                    /*
                    LONG count = 0;
                    OphirLM.GetDiffuser(hDevice, 0, count, results);
                    std::cout << "Diffuser:" << std::endl;
                    for(auto& var: results){
                        std::wcout << var << std::endl;
                    }

                    OphirLM.GetMeasurementMode(hDevice, 0, count, results);
                    std::cout << "\nMModes:" << std::endl;
                    for(auto& var: results){
                        std::wcout << var << std::endl;
                    }

                    OphirLM.GetPulseLengths(hDevice, 0, count, results);
                    std::cout << "\nPulseLengths:" << std::endl;
                    for(auto& var: results){
                        std::wcout << var << std::endl;
                    }

                    OphirLM.GetRanges(hDevice, 0, count, results);
                    std::cout << "\nRanges:" << std::endl;
                    for(auto& var: results){
                        std::wcout << var << std::endl;
                    }

                    OphirLM.GetWavelengths(hDevice, 0, count, results);
                    std::cout << "\nWls:" << std::endl;
                    for(auto& var: results){
                        std::wcout << var << std::endl;
                    }
                    */
                   
                    OphirLM.SetDiffuser(hDevice, th->channel, (long)th->diag->storage.config["laser"][0]["ophir"]["diffuser"]);
                    OphirLM.SetMeasurementMode(hDevice, th->channel, (long)th->diag->storage.config["laser"][0]["ophir"]["mMode"]);
                    OphirLM.SetPulseLength(hDevice, th->channel, (long)th->diag->storage.config["laser"][0]["ophir"]["pulseLength"]);
                    OphirLM.SetRange(hDevice, th->channel, (long)th->diag->storage.config["laser"][0]["ophir"]["range"]);
                    OphirLM.SetWavelength(hDevice, th->channel, (long)th->diag->storage.config["laser"][0]["ophir"]["wavelength"]);
                    
                    th->init = true;
                }
            }
        }catch (const _com_error& e){
            th->init = false;
            std ::wcout << L"Error 0x" << std::hex << e.Error() << L" " << e.Description()  << L"\n";
        }
        if(th->init){
            MSG Msg;
            while(!(stoken.stop_requested())){
                
                if(!th->armed.load() && th->requestArm.load()){
                    th->energy.fill(0);
                    th->times.fill(0);
                    
                    try{
                        OphirLM.ConfigureStreamMode(hDevice, th->channel, 0, 0); // (handle, channel, mode 0=turbo, true/false)
                        OphirLM.ConfigureStreamMode(hDevice, th->channel, 2, 0); // (handle, channel, mode 2=immediate, true/false)
                        //OphirLM.RegisterPlugAndPlay(PlugAndPlayCallback);	
			            //OphirLM.RegisterDataReady(DataReadyCallback);
                        OphirLM.StartStream(hDevice, th->channel);
                    }catch (const _com_error& e){
                        std ::wcout << L"Error 0x" << std::hex << e.Error() << L" " << e.Description()  << L"\n";
                    }
                
                    th->count = 0;
                    th->requestArm = false;
                    th->armed = true;
                }
                if(th->armed.load()){
                    try{
                        /*
                        //GetMessage(&Msg, NULL, 0, 0);
                        //TranslateMessage(&Msg);
                        //DispatchMessage(&Msg);

                        std::vector<double> values; //Joul
                        std::vector<double> timestamps; //ms
                        std::vector<OphirLMMeasurement::Status> statuses; //0 = ok
                
                        OphirLM.GetData(hDevice, th->channel, values, timestamps, statuses);
                        
                        for(size_t i = 0; i < values.size(); ++i){
                            if(statuses[i] == OphirLMMeasurement::Status::frequency){
                                //skip trash
                            }else{
                            
                                if(statuses[i] == OphirLMMeasurement::Status::ok){
                                    th->energy[th->count.load()] = values[i];
                                }else{
                                    th->energy[th->count.load()] = -statuses[i]; //store error
                                }
                                th->times[th->count.load()] = (unsigned long int)(timestamps[i]*1000);
                                th->count++;
                                if(th->count.load() > th->diag->storage.config["laser"][0]["pulse_count"]){
                                    std::cout << "ophir self disarmed" << std::endl;
                                    th->requestDisarm = true;
                                }
                            }
                    
                            //std::wcout << L"Timestamp: " << std::fixed << std::setprecision(3) << timestamps[i]
                            //    << L" Reading: " << std::scientific << values[i] << L" Status: " << statuses[i]<< L"\n"; 
                        
                        }
                            */
                        /*
                        if(values.size()){
                            std::cout << "end of readout" << std::endl;
                        }*/
                        using namespace std::chrono_literals;
                        std::this_thread::sleep_for(100ms);
                        th->timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();             
                    }catch (const _com_error& e){
                        std ::wcout << L"Error 0x" << std::hex << e.Error() << L" " << e.Description()  << L"\n";
                    }
                    if(th->requestDisarm.load()){
                        th->requestDisarm = false;
                        try{
                            std::vector<double> values; //Joul
                            std::vector<double> timestamps; //ms
                            std::vector<OphirLMMeasurement::Status> statuses; //0 = ok
                            using namespace std::chrono_literals;
                            std::cout << "sleep for ophir buffer" << std::endl;
                            std::this_thread::sleep_for(500ms);
                            OphirLM.GetData(hDevice, th->channel, values, timestamps, statuses);
                            
                            for(size_t i = 0; i < values.size(); ++i){
                                if(statuses[i] == OphirLMMeasurement::Status::frequency){
                                    //skip trash
                                }else{
                                
                                    if(statuses[i] == OphirLMMeasurement::Status::ok){
                                        th->energy[th->count.load()] = values[i];
                                    }else{
                                        th->energy[th->count.load()] = -statuses[i]; //store error
                                    }
                                    th->times[th->count.load()] = (unsigned long int)(timestamps[i]*1000);
                                    th->count++;
                                }
                                /*
                                std::wcout << L"Timestamp: " << std::fixed << std::setprecision(3) << timestamps[i]
                                    << L" Reading: " << std::scientific << values[i] << L" Status: " << statuses[i]<< L"\n"; 
                            */
                            }
                            OphirLM.StopAllStreams(); //stop measuring
                            
                        }catch (const _com_error& e){
                            std ::wcout << L"Error 0x" << std::hex << e.Error() << L" " << e.Description()  << L"\n";
                        }
                        std::cout << "request ophir save: " << th->count.load() << " vs " << th->energy.size() << std::endl;
                        th->diag->storage.saveOphir(min(th->count.load(), th->energy.size()));
                        th->armed = false;
                    }
                }else{
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(100ms);
                    try{
                        std::wstring headSN, headType, headName;
                        OphirLM.GetSensorInfo(hDevice, th->channel, headSN, headType, headName); 
                    }catch (const _com_error& e){
                        std ::wcout << L"Error 0x" << std::hex << e.Error() << L" " << e.Description()  << L"\n";
                    } 
                    
                    th->timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                }
            }
        }
        std::cout << "Ophir end" << std::endl;
        th->init = false;
        try{
            OphirLM.StopAllStreams(); //stop measuring
            OphirLM.CloseAll(); //close device
        }catch (const _com_error& e){
            std ::wcout << L"Error 0x" << std::hex << e.Error() << L" " << e.Description()  << L"\n";
        }
    });
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
        {"ok", this->init.load()},
        {"armed", this->armed.load()},
        {"timestamp", this->timestamp.load()},
        {"curr", this->count.load()}
    });
};

void Ophir::arm(){
    if(this->init.load() && !this->armed.load()){
        this->requestArm = true;   
    }
};

void Ophir::disarm(){
    if(this->init.load() && this->armed.load()){
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(500ms);
        std::cout << "ophir got " << this->count.load() << " before disarm" << std::endl;
        this->requestDisarm = true;
    }
    this->armed = false;
};