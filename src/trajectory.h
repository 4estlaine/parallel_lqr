//
// Created by Forrest Laine on 6/19/18.
//

#ifndef MOTORBOAT_TRAJECTORY_H
#define MOTORBOAT_TRAJECTORY_H

#include <vector>
#include "eigen3/Eigen/Dense"

#include "dynamics.h"
#include "running_constraint.h"
#include "equality_constrained_running_constraint.h"
#include "endpoint_constraint.h"
#include "equality_constrained_endpoint_constraint.h"
#include "running_cost.h"
#include "terminal_cost.h"

namespace trajectory {

class Trajectory {

 public:
  Trajectory(unsigned int trajectory_length,
             unsigned int state_dimension,
             unsigned int control_dimension,
             const dynamics::Dynamics *dynamics,
             const running_constraint::RunningConstraint *running_constraint,
             const equality_constrained_running_constraint::EqualityConstrainedRunningConstraint *equality_constrained_running_constraint,
             const endpoint_constraint::EndPointConstraint *terminal_constraint,
             const equality_constrained_endpoint_constraint::EqualityConstrainedEndPointConstraint *equality_constrained_terminal_constraint,
             const equality_constrained_endpoint_constraint::EqualityConstrainedEndPointConstraint *initial_constraint,
             const running_cost::RunningCost *running_cost,
             const terminal_cost::TerminalCost *terminal_cost):
      trajectory_length(trajectory_length),
      state_dimension(state_dimension),
      control_dimension(control_dimension),
      initial_constraint_dimension(state_dimension),
      running_constraint_dimension((unsigned int) running_constraint->get_constraint_dimension()),
      ec_running_constraint_dimension((unsigned int) equality_constrained_running_constraint->get_constraint_dimension()),
      terminal_constraint_dimension((unsigned int) terminal_constraint->get_constraint_dimension()),
      ec_terminal_constraint_dimension((unsigned int) equality_constrained_terminal_constraint->get_constraint_dimension()),
      implicit_terminal_constraint_dimension((unsigned int) (equality_constrained_terminal_constraint->is_implicit() ? equality_constrained_terminal_constraint->get_constraint_dimension() : 0)),
      dynamics(*dynamics),
      running_constraint(*running_constraint),
      equality_constrained_running_constraint(*equality_constrained_running_constraint),
      terminal_constraint(*terminal_constraint),
      equality_constrained_terminal_constraint(*equality_constrained_terminal_constraint),
      initial_constraint(*initial_constraint),
      running_cost(*running_cost),
      terminal_cost(*terminal_cost),
      current_points(trajectory_length, Eigen::VectorXd::Zero(state_dimension)),
      current_controls(trajectory_length - 1, Eigen::VectorXd::Zero(control_dimension)),
      open_loop_states(trajectory_length, Eigen::VectorXd::Zero(state_dimension)),
      open_loop_controls(trajectory_length - 1, Eigen::VectorXd::Zero(control_dimension)),
      sanity_states(trajectory_length, Eigen::VectorXd::Zero(state_dimension)),
      sanity_controls(trajectory_length - 1, Eigen::VectorXd::Zero(control_dimension)),
      num_active_constraints(trajectory_length, 0),
      active_running_constraints(trajectory_length - 1, Eigen::Matrix<bool, Eigen::Dynamic, 1>::Zero(running_constraint_dimension)),
      active_terminal_constraints(Eigen::Matrix<bool, Eigen::Dynamic, 1>::Zero(state_dimension)),
      auxiliary_constraints_present(trajectory_length, false),
      need_dynamics_mult(trajectory_length, false),
      implicit_terminal_terms_needed(trajectory_length, terminal_constraint->is_implicit()),
      running_constraint_multipliers(trajectory_length - 1, Eigen::VectorXd::Zero(running_constraint_dimension)),
      terminal_constraint_multiplier(Eigen::VectorXd::Zero(state_dimension)),
      dynamics_multipliers(trajectory_length, Eigen::VectorXd::Zero(state_dimension)),
      lq_running_constraint_multipliers(trajectory_length - 1, Eigen::VectorXd::Zero(running_constraint_dimension)),
      lq_terminal_constraint_multiplier(Eigen::VectorXd::Zero(state_dimension)),
      lq_dynamics_multipliers(trajectory_length, Eigen::VectorXd::Zero(state_dimension)),
      dynamics_jacobians_state(trajectory_length - 1, Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      dynamics_jacobians_control(trajectory_length - 1, Eigen::MatrixXd::Zero(state_dimension, control_dimension)),
      dynamics_affine_terms(trajectory_length -1, Eigen::VectorXd::Zero(state_dimension)),
      initial_constraint_jacobian_state(Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      initial_constraint_affine_term(Eigen::VectorXd::Zero(state_dimension)),
      running_constraint_jacobians_state(trajectory_length - 1, Eigen::MatrixXd::Zero(running_constraint_dimension, state_dimension)),
      running_constraint_jacobians_control(trajectory_length - 1, Eigen::MatrixXd::Zero(running_constraint_dimension, control_dimension)),
      running_constraint_affine_terms(trajectory_length - 1, Eigen::VectorXd::Zero(running_constraint_dimension)),
      ec_running_constraint_jacobians_state(trajectory_length - 1, Eigen::MatrixXd::Zero(ec_running_constraint_dimension, state_dimension)),
      ec_running_constraint_jacobians_control(trajectory_length - 1, Eigen::MatrixXd::Zero(ec_running_constraint_dimension, control_dimension)),
      ec_running_constraint_affine_terms(trajectory_length - 1, Eigen::VectorXd::Zero(ec_running_constraint_dimension)),
      active_running_constraint_jacobians_state(trajectory_length - 1, Eigen::MatrixXd::Zero(ec_running_constraint_dimension, state_dimension)),
      active_running_constraint_jacobians_control(trajectory_length - 1, Eigen::MatrixXd::Zero(ec_running_constraint_dimension, control_dimension)),
      active_running_constraint_affine_terms(trajectory_length - 1, Eigen::VectorXd::Zero(ec_running_constraint_dimension)),
      terminal_constraint_jacobian_state(Eigen::MatrixXd::Zero(terminal_constraint_dimension, state_dimension)),
      terminal_constraint_jacobian_terminal_projection(Eigen::MatrixXd::Zero(terminal_constraint_dimension, state_dimension)),
      terminal_constraint_affine_term(Eigen::VectorXd::Zero(terminal_constraint_dimension)),
      ec_terminal_constraint_jacobian_state(Eigen::MatrixXd::Zero(ec_terminal_constraint_dimension, state_dimension)),
      ec_terminal_constraint_jacobian_terminal_projection(Eigen::MatrixXd::Zero(ec_terminal_constraint_dimension, state_dimension)),
      ec_terminal_constraint_affine_term(Eigen::VectorXd::Zero(ec_terminal_constraint_dimension)),
      active_terminal_constraint_jacobian_state(Eigen::MatrixXd::Zero(ec_terminal_constraint_dimension, state_dimension)),
      active_terminal_constraint_jacobian_terminal_projection(Eigen::MatrixXd::Zero(ec_terminal_constraint_dimension, state_dimension)),
      active_terminal_constraint_affine_term(Eigen::VectorXd::Zero(ec_terminal_constraint_dimension)),
      lq_initial_state(Eigen::VectorXd::Zero(state_dimension)),
      lq_terminal_state(Eigen::VectorXd::Zero(state_dimension)),
      hamiltonian_hessians_state_state(trajectory_length - 1, Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      hamiltonian_hessians_control_state(trajectory_length - 1, Eigen::MatrixXd::Zero(control_dimension, state_dimension)),
      hamiltonian_hessians_control_control(trajectory_length - 1, Eigen::MatrixXd::Zero(control_dimension, control_dimension)),
      hamiltonian_gradients_state(trajectory_length - 1, Eigen::VectorXd::Zero(state_dimension)),
      hamiltonian_gradients_control(trajectory_length - 1, Eigen::VectorXd::Zero(control_dimension)),
      terminal_cost_hessians_state_state(Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      terminal_cost_gradient_state(Eigen::VectorXd::Zero(state_dimension)),
      current_state_feedback_matrices(trajectory_length - 1, Eigen::MatrixXd::Zero(control_dimension, state_dimension)),
      terminal_state_feedback_matrices(trajectory_length - 1, Eigen::MatrixXd::Zero(control_dimension, state_dimension)),
      feedforward_controls(trajectory_length - 1, Eigen::VectorXd::Zero(control_dimension)),
      residual_initial_constraint_jacobian_initial_state(Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      residual_initial_constraint_jacobian_terminal_projection(Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      residual_initial_constraint_affine_term(Eigen::VectorXd::Zero(state_dimension)),
      state_dependencies_initial_state_projection(trajectory_length, Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      state_dependencies_terminal_state_projection(trajectory_length, Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      state_dependencies_affine_term(trajectory_length, Eigen::VectorXd::Zero(state_dimension)),
      control_dependencies_initial_state_projection(trajectory_length-1, Eigen::MatrixXd::Zero(control_dimension, state_dimension)),
      control_dependencies_terminal_state_projection(trajectory_length-1, Eigen::MatrixXd::Zero(control_dimension, state_dimension)),
      control_dependencies_affine_term(trajectory_length-1, Eigen::VectorXd::Zero(control_dimension)),
      terminal_constraint_mult_terminal_state_feedback_term(Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      terminal_constraint_mult_initial_state_feedback_term(Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      terminal_constraint_mult_dynamics_mult_feedback_term(Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      terminal_constraint_mult_feedforward_term(Eigen::VectorXd::Zero(state_dimension)),
      running_constraint_mult_terminal_state_feedback_terms(trajectory_length - 1, Eigen::MatrixXd::Zero(ec_running_constraint_dimension, state_dimension)),
      running_constraint_mult_initial_state_feedback_terms(trajectory_length - 1, Eigen::MatrixXd::Zero(ec_running_constraint_dimension, state_dimension)),
      running_constraint_mult_dynamics_mult_feedback_terms(trajectory_length - 1, Eigen::MatrixXd::Zero(ec_running_constraint_dimension, state_dimension)),
      running_constraint_mult_feedforward_terms(trajectory_length - 1, Eigen::VectorXd::Zero(ec_running_constraint_dimension)),
      dynamics_mult_terminal_state_feedback_terms(trajectory_length, Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      dynamics_mult_initial_state_feedback_terms(trajectory_length, Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      dynamics_mult_dynamics_mult_feedback_terms(trajectory_length, Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      dynamics_mult_feedforward_terms(trajectory_length, Eigen::VectorXd::Zero(state_dimension)),
      initial_constraint_mult_terminal_state_feedback_term(Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      initial_constraint_mult_initial_state_feedback_term(Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      initial_constraint_mult_feedforward_term(Eigen::VectorXd::Zero(state_dimension)),
      cost_to_go_hessians_state_state(trajectory_length, Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      cost_to_go_hessians_terminal_terminal(trajectory_length, Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      cost_to_go_hessians_terminal_state(trajectory_length, Eigen::MatrixXd::Zero(state_dimension, state_dimension)),
      cost_to_go_gradients_state(trajectory_length, Eigen::VectorXd::Zero(state_dimension)),
      cost_to_go_gradients_terminal(trajectory_length, Eigen::VectorXd::Zero(state_dimension)),
      cost_to_go_offsets(trajectory_length, Eigen::MatrixXd::Zero(1, 1)),
      A(Eigen::MatrixXd::Zero(0,0)),
      AB(0),
      B(0) {};

  ~Trajectory() = default;

  void execute_current_control_policy(double disturbance);

  void execute_open_loop_policy();

  void execute_sanity_policy();

  void populate_derivative_terms();

  void update_working_set();

  void populate_bandsolve_terms();

  void bandsolve_traj();

  void extract_bandsoln();

  void compute_feedback_policies();

  void perform_constrained_dynamic_programming_backup(const Eigen::MatrixXd *Qxx,
                                                      const Eigen::MatrixXd *Quu,
                                                      const Eigen::MatrixXd *Qux,
                                                      const Eigen::VectorXd *Qx,
                                                      const Eigen::VectorXd *Qu,
                                                      const Eigen::MatrixXd *Ax,
                                                      const Eigen::MatrixXd *Au,
                                                      const Eigen::VectorXd *A1,
                                                      const Eigen::MatrixXd *Dx,
                                                      const Eigen::MatrixXd *Du,
                                                      const Eigen::VectorXd *D1,
                                                      unsigned int num_active_constraints,
                                                      bool implicit_terminal_terms_needed,
                                                      Eigen::MatrixXd &Mxx,
                                                      Eigen::MatrixXd &Muu,
                                                      Eigen::MatrixXd &Mzz,
                                                      Eigen::MatrixXd &Mux,
                                                      Eigen::MatrixXd &Mzx,
                                                      Eigen::MatrixXd &Mzu,
                                                      Eigen::VectorXd &Mx1,
                                                      Eigen::VectorXd &Mu1,
                                                      Eigen::VectorXd &Mz1,
                                                      Eigen::MatrixXd &M11,
                                                      Eigen::VectorXd &N1,
                                                      Eigen::MatrixXd &Nx,
                                                      Eigen::MatrixXd &Nu,
                                                      Eigen::MatrixXd &Nz,
                                                      Eigen::MatrixXd &Vxx,
                                                      Eigen::MatrixXd &Vzz,
                                                      Eigen::MatrixXd &Vzx,
                                                      Eigen::VectorXd &Vx1,
                                                      Eigen::VectorXd &Vz1,
                                                      Eigen::MatrixXd &V11,
                                                      Eigen::MatrixXd &Gx,
                                                      Eigen::MatrixXd &Gz,
                                                      Eigen::VectorXd &G1,
                                                      Eigen::MatrixXd &Lx,
                                                      Eigen::MatrixXd &Lz,
                                                      Eigen::VectorXd &L1);

  void compute_state_control_dependencies();

  void set_open_loop_traj();

  void set_lq_multipliers();

  void compute_multipliers();

  // Setters

  void set_initial_constraint_dimension(unsigned int d);

  void set_initial_constraint_jacobian_state(const Eigen::MatrixXd* H);

  void set_initial_constraint_affine_term(const Eigen::VectorXd* h);

  void set_terminal_constraint_dimension(unsigned int d);

  void set_terminal_constraint_jacobian_state(const Eigen::MatrixXd* H);

  void set_terminal_constraint_jacobian_terminal_projection(const Eigen::MatrixXd* H);

  void set_terminal_constraint_affine_term(const Eigen::VectorXd* h);

  void set_dynamics_jacobian_state(unsigned int t, const Eigen::MatrixXd* A);

  void set_dynamics_jacobian_control(unsigned int t, const Eigen::MatrixXd* B);

  void set_dynamics_affine_term(unsigned int t, const Eigen::VectorXd* c);

  void set_num_active_constraints(unsigned int t, unsigned int num_active_constraints);

  void set_hamiltonian_hessians_state_state(unsigned int t, const Eigen::MatrixXd* Qxx);

  void set_hamiltonian_hessians_control_state(unsigned int t, const Eigen::MatrixXd* Qux);

  void set_hamiltonian_hessians_control_control(unsigned int t, const Eigen::MatrixXd* Quu);

  void set_hamiltonian_gradients_state(unsigned int t, const Eigen::VectorXd* Qx);

  void set_hamiltonian_gradients_control(unsigned int t, const Eigen::VectorXd* Qu);

  void set_terminal_cost_hessians_state_state(const Eigen::MatrixXd* Qxx);

  void set_terminal_cost_gradient_state(const Eigen::VectorXd* Qx);

  void set_terminal_point(Eigen::VectorXd *terminal_projection);

  void set_initial_point(Eigen::VectorXd *initial_projection);

  // Getters

  void get_state_dependencies_initial_state_projection(unsigned int t, Eigen::MatrixXd& Tx) const;

  void get_state_dependencies_terminal_state_projection(unsigned int t, Eigen::MatrixXd& Tz) const;

  void get_state_dependencies_affine_term(unsigned int t, Eigen::VectorXd& T1) const;

  void get_control_dependencies_initial_state_projection(unsigned int t, Eigen::MatrixXd& Tx) const;

  void get_control_dependencies_terminal_state_projection(unsigned int t, Eigen::MatrixXd& Tz) const;

  void get_control_dependencies_affine_term(unsigned int t, Eigen::VectorXd& T1) const;

  void get_dynamics_mult_initial_state_feedback_term(unsigned int t, Eigen::MatrixXd& Tx) const;

  void get_dynamics_mult_terminal_state_feedback_term(unsigned int t, Eigen::MatrixXd& Tz) const;

  void get_dynamics_mult_feedforward_term(unsigned int t, Eigen::VectorXd& T1) const;

  void get_running_constraint_mult_initial_state_feedback_term(unsigned int t, Eigen::MatrixXd& Tx) const;

  void get_running_constraint_mult_terminal_state_feedback_term(unsigned int t, Eigen::MatrixXd& Tz) const;

  void get_running_constraint_mult_feedforward_term(unsigned int t, Eigen::VectorXd& T1) const;

  void get_terminal_constraint_mult_initial_state_feedback_term(Eigen::MatrixXd& Tx) const;

  void get_terminal_constraint_mult_terminal_state_feedback_term(Eigen::MatrixXd& Tz) const;

  void get_terminal_constraint_mult_feedforward_term(Eigen::VectorXd& T1) const;


 public:
  const unsigned int trajectory_length;
  const unsigned int state_dimension;
  const unsigned int control_dimension;
  unsigned int initial_constraint_dimension;
  const unsigned int running_constraint_dimension;
  const unsigned int ec_running_constraint_dimension;
  unsigned int terminal_constraint_dimension;
  unsigned int ec_terminal_constraint_dimension;
  unsigned int implicit_terminal_constraint_dimension;

  dynamics::Dynamics dynamics;
  running_constraint::RunningConstraint running_constraint;
  equality_constrained_running_constraint::EqualityConstrainedRunningConstraint equality_constrained_running_constraint;
  endpoint_constraint::EndPointConstraint terminal_constraint;
  equality_constrained_endpoint_constraint::EqualityConstrainedEndPointConstraint equality_constrained_terminal_constraint;
  equality_constrained_endpoint_constraint::EqualityConstrainedEndPointConstraint initial_constraint;
  running_cost::RunningCost running_cost;
  terminal_cost::TerminalCost terminal_cost;

  std::vector<Eigen::VectorXd> current_points;
  std::vector<Eigen::VectorXd> current_controls;

  std::vector<Eigen::VectorXd> open_loop_states;
  std::vector<Eigen::VectorXd> open_loop_controls;

  std::vector<Eigen::VectorXd> sanity_states;
  std::vector<Eigen::VectorXd> sanity_controls;

  std::vector<unsigned int> num_active_constraints;
  unsigned int num_active_terminal_constraints;
  std::vector<Eigen::Matrix<bool, Eigen::Dynamic, 1>> active_running_constraints;
  Eigen::Matrix<bool, Eigen::Dynamic, 1> active_terminal_constraints;
  std::vector<bool> auxiliary_constraints_present;
  std::vector<bool> need_dynamics_mult;
  std::vector<bool> implicit_terminal_terms_needed;

  std::vector<Eigen::VectorXd> running_constraint_multipliers;
  Eigen::VectorXd terminal_constraint_multiplier;
  std::vector<Eigen::VectorXd> dynamics_multipliers;

  std::vector<Eigen::VectorXd> lq_running_constraint_multipliers;
  Eigen::VectorXd lq_terminal_constraint_multiplier;
  std::vector<Eigen::VectorXd> lq_dynamics_multipliers;

  std::vector<Eigen::MatrixXd> dynamics_jacobians_state;
  std::vector<Eigen::MatrixXd> dynamics_jacobians_control;
  std::vector<Eigen::VectorXd> dynamics_affine_terms;

  Eigen::MatrixXd initial_constraint_jacobian_state;
  Eigen::VectorXd initial_constraint_affine_term;

  std::vector<Eigen::MatrixXd> running_constraint_jacobians_state;
  std::vector<Eigen::MatrixXd> running_constraint_jacobians_control;
  std::vector<Eigen::VectorXd> running_constraint_affine_terms;

  std::vector<Eigen::MatrixXd> ec_running_constraint_jacobians_state;
  std::vector<Eigen::MatrixXd> ec_running_constraint_jacobians_control;
  std::vector<Eigen::VectorXd> ec_running_constraint_affine_terms;

  std::vector<Eigen::MatrixXd> active_running_constraint_jacobians_state;
  std::vector<Eigen::MatrixXd> active_running_constraint_jacobians_control;
  std::vector<Eigen::VectorXd> active_running_constraint_affine_terms;

  Eigen::MatrixXd terminal_constraint_jacobian_state;
  Eigen::MatrixXd terminal_constraint_jacobian_terminal_projection;
  Eigen::VectorXd terminal_constraint_affine_term;

  Eigen::MatrixXd ec_terminal_constraint_jacobian_state;
  Eigen::MatrixXd ec_terminal_constraint_jacobian_terminal_projection;
  Eigen::VectorXd ec_terminal_constraint_affine_term;

  Eigen::MatrixXd active_terminal_constraint_jacobian_state;
  Eigen::MatrixXd active_terminal_constraint_jacobian_terminal_projection;
  Eigen::VectorXd active_terminal_constraint_affine_term;

  Eigen::VectorXd lq_initial_state;
  Eigen::VectorXd lq_terminal_state;

  std::vector<Eigen::MatrixXd> hamiltonian_hessians_state_state;
  std::vector<Eigen::MatrixXd> hamiltonian_hessians_control_state;
  std::vector<Eigen::MatrixXd> hamiltonian_hessians_control_control;

  std::vector<Eigen::VectorXd> hamiltonian_gradients_state;
  std::vector<Eigen::VectorXd> hamiltonian_gradients_control;

  Eigen::MatrixXd terminal_cost_hessians_state_state;
  Eigen::VectorXd terminal_cost_gradient_state;

  std::vector<Eigen::MatrixXd> current_state_feedback_matrices;
  std::vector<Eigen::MatrixXd> terminal_state_feedback_matrices;
  std::vector<Eigen::VectorXd> feedforward_controls;

  Eigen::MatrixXd residual_initial_constraint_jacobian_initial_state;
  Eigen::MatrixXd residual_initial_constraint_jacobian_terminal_projection;
  Eigen::VectorXd residual_initial_constraint_affine_term;

  std::vector<Eigen::MatrixXd> state_dependencies_initial_state_projection;
  std::vector<Eigen::MatrixXd> state_dependencies_terminal_state_projection;
  std::vector<Eigen::VectorXd> state_dependencies_affine_term;
  std::vector<Eigen::MatrixXd> control_dependencies_initial_state_projection;
  std::vector<Eigen::MatrixXd> control_dependencies_terminal_state_projection;
  std::vector<Eigen::VectorXd> control_dependencies_affine_term;

  Eigen::MatrixXd terminal_constraint_mult_terminal_state_feedback_term;
  Eigen::MatrixXd terminal_constraint_mult_initial_state_feedback_term;
  Eigen::MatrixXd terminal_constraint_mult_dynamics_mult_feedback_term;
  Eigen::VectorXd terminal_constraint_mult_feedforward_term;

  std::vector<Eigen::MatrixXd> running_constraint_mult_terminal_state_feedback_terms;
  std::vector<Eigen::MatrixXd> running_constraint_mult_initial_state_feedback_terms;
  std::vector<Eigen::MatrixXd> running_constraint_mult_dynamics_mult_feedback_terms;
  std::vector<Eigen::VectorXd> running_constraint_mult_feedforward_terms;

  std::vector<Eigen::MatrixXd> dynamics_mult_terminal_state_feedback_terms;
  std::vector<Eigen::MatrixXd> dynamics_mult_initial_state_feedback_terms;
  std::vector<Eigen::MatrixXd> dynamics_mult_dynamics_mult_feedback_terms;
  std::vector<Eigen::VectorXd> dynamics_mult_feedforward_terms;

  Eigen::MatrixXd initial_constraint_mult_terminal_state_feedback_term;
  Eigen::MatrixXd initial_constraint_mult_initial_state_feedback_term;
  Eigen::VectorXd initial_constraint_mult_feedforward_term;

  std::vector<Eigen::MatrixXd> cost_to_go_hessians_state_state;
  std::vector<Eigen::MatrixXd> cost_to_go_hessians_terminal_terminal;
  std::vector<Eigen::MatrixXd> cost_to_go_hessians_terminal_state;
  std::vector<Eigen::VectorXd> cost_to_go_gradients_state;
  std::vector<Eigen::VectorXd> cost_to_go_gradients_terminal;
  std::vector<Eigen::MatrixXd> cost_to_go_offsets;

  Eigen::PermutationMatrix<Eigen::Dynamic, Eigen::Dynamic, int> perm;

  //todo: Clean this crap up
  Eigen::MatrixXd A;
  std::vector<double> AB, B;

  int half_bandwidth = 0;
  int soln_size = 0;

};
}  // namespace trajectory

#endif //MOTORBOAT_TRAJECTORY_H
