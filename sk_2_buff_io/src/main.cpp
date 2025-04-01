#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <thread> 
#include <iostream>
#include <fstream>

const std::string FILE_NAME     = "usrp_samples.csv";   // Имя CSV файла для приёма
const size_t      DURATION_SECS = 5;                    // Длительность сигнала (сек)

void transmit_one_burst(
    uhd::tx_streamer::sptr tx_stream,
    short* tx_buff,
    size_t num_samples,
    double delay_sec = 0.5
);

void receive_to_csv(
    uhd::rx_streamer::sptr rx_stream,
    const std::string& file_name,
    short* rx_buff,
    size_t total_num_samps,
    size_t samps_per_buff,
    double settling_time = 0.5
);

/**
 * 
 * 
 */
int UHD_SAFE_MAIN(int argc, char* argv[]) {
    /**
     * First Device constructor
     * Создаём 2 объекта tx и rx
     */
    uhd::device_addr_t dev_args("serial=3457573"); // From uhd_usrp_probe

    /**
     * Empty var for usrp
     */
    uhd::usrp::multi_usrp::sptr usrp = nullptr; 

    try { 
        usrp = uhd::usrp::multi_usrp::make(dev_args);
    } catch(const uhd::exception& e) { 
        std::cout << "Some excpt[" << e.code() << "]: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    
    /** 
     * get_mboard_name()
     * Description: Определяет название устройства mboard - mother-board
     */
    std::string mboard_name     = usrp->get_mboard_name(); 
    size_t      mboard_numbers  = usrp->get_num_mboards();

    std::cout << "mboard_name: " << mboard_name << "\n";
    std::cout << "mboard_count: " << mboard_numbers << "\n"; 

    /**
     * Clock
     */
    usrp->set_clock_source("internal"); // from uhd_usrp_probe: [Clock sources: internal, external, gpsdo]

    /**
     * Get channels
     */
    size_t tx_channels = usrp->get_tx_num_channels(); 
    size_t rx_channels = usrp->get_rx_num_channels(); 

    std::cout << "Tx channels: " << tx_channels << "\n"; 
    std::cout << "Rx channels: " << rx_channels << "\n"; 
    /**
     * Getting subdev_name for all rx/tx channels
     */
    for (size_t tx_c = 0; tx_c < tx_channels; tx_c++) { 
        std::cout << "Tx subdev name for chan[" << tx_c << "]: " << usrp->get_tx_subdev_name(tx_c) << "\n";
    }

    for (size_t rx_c = 0; rx_c < rx_channels; rx_c++) { 
        std::cout << "Rx subdev name for chan[" << rx_c << "]: " << usrp->get_rx_subdev_name(rx_c) << "\n";
    }

    for (size_t mbc = 0; mbc < mboard_numbers; mbc++) { 
        uhd::usrp::subdev_spec_t sdev_spec_rx = usrp->get_rx_subdev_spec(mbc);
        uhd::usrp::subdev_spec_t sdev_spec_tx = usrp->get_tx_subdev_spec(mbc);

        std::cout << sdev_spec_rx.to_pp_string() << "\n";
        std::cout << sdev_spec_tx.to_pp_string() << "\n";
    } 

    /** 
     *  Select the subdevice first!!
     *  uhd::usrp::subdev_spec_t tx_spec("daughterboard:subdevice"); 
    */
    uhd::usrp::subdev_spec_t tx_spec("A:A"); 
    usrp->set_tx_subdev_spec(tx_spec);

    uhd::usrp::subdev_spec_t rx_spec("A:A"); 
    usrp->set_rx_subdev_spec(rx_spec);

    /**
     * Кол-во каналов не меняется
     */
    // size_t tx_channels_after_subdev = usrp->get_tx_num_channels(); 
    // size_t rx_channels_after_subdev = usrp->get_rx_num_channels(); 

    // std::cout << "Tx channels: " << tx_channels_after_subdev << "\n"; 
    // std::cout << "Rx channels: " << rx_channels_after_subdev << "\n"; 
    // /**
    //  * Getting subdev_name for all rx/tx channels
    //  */
    // for (size_t tx_c = 0; tx_c < tx_channels_after_subdev; tx_c++) { 
    //     std::cout << "Tx subdev name for chan[" << tx_c << "]: " << usrp->get_tx_subdev_name(tx_c) << "\n";
    // }

    // for (size_t rx_c = 0; rx_c < rx_channels_after_subdev; rx_c++) { 
    //     std::cout << "Rx subdev name for chan[" << rx_c << "]: " << usrp->get_rx_subdev_name(rx_c) << "\n";
    // }

    /**
     * Установка тактовой частоты 
     * И установка Sample Rate для TX и RX потока
     */
    std::cout << "Using SubDevices: " << usrp->get_pp_string() << std::endl;

    // Ведущая(главная) частота ADC/DAC (необязательный параметр)
    // https://uhd.readthedocs.io/en/uhd-4.1/classuhd_1_1usrp_1_1multi__usrp.html#a99254abfa5259b70a020e667eee619b9
    constexpr double master_clock_rate = 16e6;
    
    // tx_srate должна быть вдвое меньше! (но можно любое число до 8e6 включительно)
    constexpr double tx_srate          = master_clock_rate / 16.0; 
    constexpr double rx_srate          = master_clock_rate / 16.0; 

    usrp->set_master_clock_rate(master_clock_rate); 
    usrp->set_tx_rate(tx_srate); // 8MSps (16e6 / 2)
    usrp->set_rx_rate(rx_srate); 
    

    // Валидация изменений master clock
    std::cout << "Master clock: " << (usrp->get_master_clock_rate() / 1e6) << " MHz\n";

    // [tx_channels - 1] - номер канала (он у нас нулевой) 
    std::cout << "Tx Srate: " << (usrp->get_tx_rate(tx_channels - 1) / 1e6) << " MHz\n";
    std::cout << "Rx Srate: " << (usrp->get_rx_rate(rx_channels - 1) / 1e6) << " MHz\n"; 

    double txrx_central_freq = 915e6;

    uhd::tune_request_t tx_freq_request(txrx_central_freq);
    uhd::tune_result_t  tx_freq_change_result = usrp->set_tx_freq(tx_freq_request); 

    uhd::tune_request_t rx_freq_request(txrx_central_freq); 
    uhd::tune_result_t  rx_freq_change_result = usrp->set_rx_freq(rx_freq_request); 

    double tx_gain = 50; // dB
    usrp->set_tx_gain(tx_gain);

    double rx_gain = 20; // dB
    usrp->set_rx_gain(rx_gain);
    
    std::cout << "Tx Available antenas: \n";  
    auto tx_antenas = usrp->get_tx_antennas();
    for(auto ant{tx_antenas.begin()}; ant !=tx_antenas.end(); ant++ )
    {
        std::cout << *ant << std::endl;
    }

    std::cout << "Rx Available antenas: \n";  
    auto rx_antenas = usrp->get_rx_antennas();
    for(auto ant{rx_antenas.begin()}; ant !=rx_antenas.end(); ant++ )
    {
        std::cout << *ant << std::endl;
    }

    /**
     * Как я понял можно передачу/приём в двух каналах вести, либо полный дуплекс! 
     */
    // Обе антены могут вещать, и читать канал
    usrp->set_tx_antenna("TX/RX");
    usrp->set_rx_antenna("TX/RX"); 

    // Необязательно
    // usrp->set_rx_bandwidth(18e6);  // [0.2MHz - 56MHz]
    // usrp->set_tx_bandwidth(18e6);

    /** 
     *  Setup Streaming
    */
    
    /***
     * 1 - формат хранения данных на хосте (до передачи) (CPU)
     * 2 - формат передачи данных OTW - over-the-wire
     * 
     * https://uhd.readthedocs.io/en/uhd-4.1/structuhd_1_1stream__args__t.html#a602a64b4937a85dba84e7f724387e252
     * https://uhd.readthedocs.io/en/uhd-4.1/structuhd_1_1stream__args__t.html#a0ba0e946d2f83f7ac085f4f4e2ce9578
     */
    uhd::stream_args_t stream_args; 
    // Для упрощения предположим, что наш буфер на ПК хронит sc16, так и будем передавать sc16
    stream_args.cpu_format = "sc16"; // short complex 
    stream_args.otw_format = "sc16"; // short complex
    stream_args.channels   = std::vector<size_t>{0}; 


    // Stream object
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);  
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    // Количество значений на цикл передачи (пакет)
    size_t max_element_to_transmit = tx_stream->get_max_num_samps();

    // channel ~ buffer 
    // channel num = 1 => мы можем передать максимально 2040 sc16 / для sc8 это кол-во в 2 раза больше
    std::cout << "max number of samples per buffer per packet: " << max_element_to_transmit << "\n";
    
    /**
     *  sc16 reference 
     *  Q_16, I_16
     *  https://uhd.readthedocs.io/en/uhd-4.1/structuhd_1_1stream__args__t.html#a0ba0e946d2f83f7ac085f4f4e2ce9578 
     *  */    

    constexpr size_t signal_length = static_cast<size_t>(tx_srate * DURATION_SECS);
    
    short* buff_out         = new short[signal_length]; // буфер передачи
    short* buff_in          = new short[signal_length]; // буфер приёма

    // да, можно было через memset
    for (size_t i = 0; i < signal_length; i+=2) { 
        // Для уменьшения ошибок квантования подаём верхнее значение (чтоб уж наверняка)
        short Q_val = 0x7FFF; 
        short I_val = 0x7FFF; 

        buff_out[i]     = Q_val; 
        buff_out[i + 1] = I_val;  

        buff_in[i] = 0;
        buff_in[i + 1] = 0; 
    }

    // Сбрасываем часы, обозначая передачу
    usrp->set_time_now(uhd::time_spec_t(0.0));

    //txrx

    //тут можно поработать с передачей. 
    //есть вероятность, что я ошибся насчёт mux_num_samples
    transmit_one_burst(tx_stream, buff_out, signal_length);
    receive_to_csv(rx_stream, FILE_NAME, buff_in, signal_length, max_element_to_transmit);

    // unsafe?
    // delete[] buff_in;
    // delete[] buff_out;

    return EXIT_SUCCESS;
}

void transmit_one_burst(
    uhd::tx_streamer::sptr tx_stream,
    short* tx_buff,
    size_t num_samples,
    double delay_sec
) {
    uhd::tx_metadata_t tx_md;
    tx_md.start_of_burst = true;
    tx_md.end_of_burst   = true;
    tx_md.has_time_spec  = true;
    tx_md.time_spec      = uhd::time_spec_t(delay_sec);

    std::cout << boost::format("Transmitting %u samples...\n") % num_samples;

    size_t sent_samps = tx_stream->send(&tx_buff[0], num_samples, tx_md);
    


    // if (sent_samps != num_samples) {
    //     std::cout << sent_samps;
    //     throw std::runtime_error("Failed to transmit all samples");
    // }
    
    std::cout << "Transmission complete.\n";
}

// Функция приёма с использованием short*
void receive_to_csv(
    uhd::rx_streamer::sptr rx_stream,
    const std::string& file_name,
    short* rx_buff,
    size_t total_num_samps,
    size_t samps_per_buff,
    double settling_time
) {
    std::ofstream outfile(file_name);
    
    if (!outfile.is_open()) {
        throw std::runtime_error("Cannot open CSV file for writing.");
    }

    // Записываем заголовок CSV
    outfile << "Q,I\n";

    // Настраиваем команду стриминга
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps   = total_num_samps;
    stream_cmd.stream_now  = false;
    stream_cmd.time_spec   = uhd::time_spec_t(settling_time);
    
    rx_stream->issue_stream_cmd(stream_cmd);

    uhd::rx_metadata_t md;
    
    size_t total_received = 0;

    std::cout << "Receiving " << total_num_samps << " samples...\n";

    while (total_received < total_num_samps) {
        // Принимаем данные в буфер, интерпретируя их как std::complex<short>
        size_t num_rx_samps = rx_stream->recv(
            reinterpret_cast<std::complex<short>*>(&rx_buff[total_received * 2]),
            samps_per_buff,
            md,
            1.0
        );

        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
            std::cerr << "Timeout while streaming.\n";
            break;
        }

        if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
            throw std::runtime_error("Receiver error: " + md.strerror());
        }

        // Записываем полученные данные в CSV файл
        for (size_t i = 0; i < num_rx_samps; i++) {
            outfile << rx_buff[(total_received + i) * 2 + 1] << "," // Q
                    << rx_buff[(total_received + i) * 2] << "\n";   // I
        }

        total_received += num_rx_samps;

        if (md.end_of_burst) {
            std::cout << "End of burst received.\n";
            break;
        }
        
        // if (stop_signal_called) {
        //     std::cout << "Stop signal called. Exiting reception loop.\n";
        //     break;
        // }
    }

    // Завершаем стриминг
    uhd::stream_cmd_t stop_stream(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
    rx_stream->issue_stream_cmd(stop_stream);

    outfile.close();
    
    std::cout << "Reception complete. Received " << total_received 
              << " samples. Data saved to " << file_name << "\n";
}
