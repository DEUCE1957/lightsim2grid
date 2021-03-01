// Copyright (c) 2020, RTE (https://www.rte-france.com)
// See AUTHORS.txt
// This Source Code Form is subject to the terms of the Mozilla Public License, version 2.0.
// If a copy of the Mozilla Public License, version 2.0 was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
// This file is part of LightSim2grid, LightSim2grid implements a c++ backend targeting the Grid2Op platform.

#include "DataSGen.h"
void DataSGen::init(const RealVect & sgen_p,
                    const RealVect & sgen_q,
                    const RealVect & sgen_pmin,
                    const RealVect & sgen_pmax,
                    const RealVect & sgen_qmin,
                    const RealVect & sgen_qmax,
                    const Eigen::VectorXi & sgen_bus_id)
{
    int size = sgen_p.size();
    DataGeneric::check_size(sgen_p, size, "sgen_p");
    DataGeneric::check_size(sgen_q, size, "sgen_q");
    DataGeneric::check_size(sgen_pmin, size, "sgen_pmin");
    DataGeneric::check_size(sgen_pmax, size, "sgen_pmax");
    DataGeneric::check_size(sgen_qmin, size, "sgen_qmin");
    DataGeneric::check_size(sgen_qmax, size, "sgen_qmax");
    DataGeneric::check_size(sgen_bus_id, size, "sgen_bus_id");

    p_mw_ = sgen_p;
    q_mvar_ = sgen_q;
    p_min_mw_ = sgen_pmin;
    p_max_mw_ = sgen_pmax;
    q_min_mvar_ = sgen_qmin;
    q_max_mvar_ = sgen_qmax;
    bus_id_ = sgen_bus_id;
    status_ = std::vector<bool>(sgen_p.size(), true);
}


DataSGen::StateRes DataSGen::get_state() const
{
     std::vector<real_type> p_mw(p_mw_.begin(), p_mw_.end());
     std::vector<real_type> q_mvar(q_mvar_.begin(), q_mvar_.end());
     std::vector<real_type> p_min(p_min_mw_.begin(), p_min_mw_.end());
     std::vector<real_type> p_max(p_max_mw_.begin(), p_max_mw_.end());
     std::vector<real_type> q_min(q_min_mvar_.begin(), q_min_mvar_.end());
     std::vector<real_type> q_max(q_max_mvar_.begin(), q_max_mvar_.end());
     std::vector<int> bus_id(bus_id_.begin(), bus_id_.end());
     std::vector<bool> status = status_;
     DataSGen::StateRes res(p_mw, q_mvar, p_min, p_max, q_min, q_max, bus_id, status);
     return res;
}
void DataSGen::set_state(DataSGen::StateRes & my_state )
{
    reset_results();

    std::vector<real_type> & p_mw = std::get<0>(my_state);
    std::vector<real_type> & q_mvar = std::get<1>(my_state);
    std::vector<real_type> & p_min = std::get<2>(my_state);
    std::vector<real_type> & p_max = std::get<3>(my_state);
    std::vector<real_type> & q_min = std::get<4>(my_state);
    std::vector<real_type> & q_max = std::get<5>(my_state);
    std::vector<int> & bus_id = std::get<6>(my_state);
    std::vector<bool> & status = std::get<7>(my_state);
    auto size = p_mw.size();
    DataGeneric::check_size(p_mw, size, "p_mw");
    DataGeneric::check_size(q_mvar, size, "q_mvar");
    DataGeneric::check_size(p_min, size, "p_min");
    DataGeneric::check_size(p_max, size, "p_max");
    DataGeneric::check_size(q_min, size, "q_min");
    DataGeneric::check_size(q_max, size, "q_max");
    DataGeneric::check_size(bus_id, size, "bus_id");
    DataGeneric::check_size(status, size, "status");

    p_mw_ = RealVect::Map(&p_mw[0], size);
    q_mvar_ = RealVect::Map(&q_mvar[0], size);
    q_mvar_ = RealVect::Map(&q_mvar[0], size);
    p_min_mw_ = RealVect::Map(&p_min[0], size);
    p_max_mw_ = RealVect::Map(&p_max[0], size);
    q_min_mvar_ = RealVect::Map(&q_min[0], size);
    q_max_mvar_ = RealVect::Map(&q_max[0], size);
    bus_id_ = Eigen::VectorXi::Map(&bus_id[0], bus_id.size());
    status_ = status;
}


void DataSGen::fillSbus(CplxVect & Sbus, bool ac, const std::vector<int> & id_grid_to_solver){
    int nb_sgen = nb();
    int bus_id_me, bus_id_solver;
    cplx_type tmp;
    for(int sgen_id = 0; sgen_id < nb_sgen; ++sgen_id){
        //  i don't do anything if the load is disconnected
        if(!status_[sgen_id]) continue;

        bus_id_me = bus_id_(sgen_id);
        bus_id_solver = id_grid_to_solver[bus_id_me];
        if(bus_id_solver == _deactivated_bus_id){
            //TODO improve error message with the gen_id
            throw std::runtime_error("One load is connected to a disconnected bus.");
        }
        tmp = static_cast<cplx_type>(p_mw_(sgen_id));
        if(ac) tmp += my_i * q_mvar_(sgen_id);
        Sbus.coeffRef(bus_id_solver) += tmp;
    }
}

void DataSGen::compute_results(const Eigen::Ref<RealVect> & Va,
                               const Eigen::Ref<RealVect> & Vm,
                               const Eigen::Ref<CplxVect> & V,
                               const std::vector<int> & id_grid_to_solver,
                               const RealVect & bus_vn_kv,
                               real_type sn_mva)
{
    int nb_sgen = nb();
    v_kv_from_vpu(Va, Vm, status_, nb_sgen, bus_id_, id_grid_to_solver, bus_vn_kv, res_v_);
    res_p_ = p_mw_;
    res_q_ = q_mvar_;
}

void DataSGen::reset_results(){
    res_p_ = RealVect();  // in MW
    res_q_ =  RealVect();  // in MVar
    res_v_ = RealVect();  // in kV
}

void DataSGen::change_p(int sgen_id, real_type new_p, bool & need_reset)
{
    bool my_status = status_.at(sgen_id); // and this check that load_id is not out of bound
    if(!my_status) throw std::runtime_error("Impossible to change the active value of a disconnected static generator");
    p_mw_(sgen_id) = new_p;
}

void DataSGen::change_q(int sgen_id, real_type new_q, bool & need_reset)
{
    bool my_status = status_.at(sgen_id); // and this check that load_id is not out of bound
    if(!my_status) throw std::runtime_error("Impossible to change the reactive value of a disconnected static generator");
    q_mvar_(sgen_id) = new_q;
}

real_type DataSGen::get_p_slack(int slack_bus_id)
{
    int nb_element = nb();
    real_type res = 0.;
    for(int sgen_id = 0; sgen_id < nb_element; ++sgen_id)
    {
        if(!status_[sgen_id]) continue;
        if(bus_id_(sgen_id) == slack_bus_id) res += res_p_(sgen_id);
    }
    return res;
}

void DataSGen::get_q(std::vector<real_type>& q_by_bus)
{
    int nb_element = nb();
    for(int sgen_id = 0; sgen_id < nb_element; ++sgen_id)
    {
        if(!status_[sgen_id]) continue;
        int bus_id = bus_id_[sgen_id];
        q_by_bus[bus_id] += res_q_(sgen_id); //TODO weird that i need to put a + here and a - for the active!
    }
}
