package org.example.backend.service;

import org.example.backend.model.Temperature;
import org.example.backend.repository.TemperatureRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import java.util.List;
import java.util.Optional;

@Service
public class TemperatureService {

    @Autowired
    private TemperatureRepository temperatureRepository;

    @Transactional
    public Temperature saveTemperature(Temperature temperature) {
        return temperatureRepository.save(temperature);
    }

    public List<Temperature> getAllTemperature() {
        return temperatureRepository.findAll();
    }

    public Optional<Temperature> getTemperatureById(Long id) {
        return temperatureRepository.findById(id);
    }

    public Optional<Temperature> getLatestTemperature() {
        return temperatureRepository.findFirstByOrderByCreatedAtDesc();
    }
}
