package org.example.backend.service;

import org.example.backend.model.VitalSigns;
import org.example.backend.repository.VitalSignsRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import java.util.List;
import java.util.Optional;

@Service
public class VitalSignsService {

    @Autowired
    private VitalSignsRepository vitalSignsRepository;

    @Transactional
    public VitalSigns saveVitalSigns(VitalSigns vitalSigns) {
        return vitalSignsRepository.save(vitalSigns);
    }

    public List<VitalSigns> getAllVitalSigns() {
        return vitalSignsRepository.findAll();
    }

    public Optional<VitalSigns> getVitalSignsById(Long id) {
        return vitalSignsRepository.findById(id);
    }

    public Optional<VitalSigns> getLatestVitalSigns() {
        return vitalSignsRepository.findFirstByOrderByCreatedAtDesc();
    }
}
