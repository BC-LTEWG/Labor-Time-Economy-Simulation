import logging
logger = logging.getLogger(__name__)
import numpy as np
from copy import deepcopy
from .Collector import Collector
from typing import Tuple
from scipy.linalg import inv, eig
from scipy.stats import norm
from pathlib import Path
import math
import copy
from overseer.tools.dataclasses import Replace, Extend, Append, Update

np.set_printoptions(
    precision=3,      # digits after decimal-ish
    suppress=True,    # avoid scientific notation when possible
    linewidth=200     # avoid wrapping rows too early
)

class Aggregator:
    """ The guy who watches the LTE and keeps track of the data. """
    def __init__(self, bin_path, params):
        self.params = params

        # create a more human readable dictionary of helpful quantities
        self.settings = {
            "n_goods": params.N_g,
            "n_machines": params.N_m,
            "max_num_inputs_per_good": params.N_inputs_max,
            "n_time_steps": params.N_S,
            "n_persons": params.N_h,
            # "prods_per_machine": params.m_r,
            "fixed_seed": params.fixed_seed,
            "seed": params.seed,
            "n_producers": params.N_p,
            "n_distributors": params.N_d,
            "n_firms": params.N_p + params.N_d,
            "init_working_day": params.T,
            "init_working_week": params.W,
            "daily_sick_chance": params.S,
            "person_ability_stddev": params.v_ability,
            "n_abilities": params.N_a,
            "productivity": params.productivity,
            "consump_epsilon": params.consump_epsilon,
            # the rest of these are d if pending_inventory_j else 0ependent variables
            "n_sectors": params.N_g + params.N_m + 1,
            "n_consumer_goods": params.N_g,
            "n_products": 2 * params.N_g + params.N_m,
            "init_prices": params.init_prices,
            "free_goods": params.free_goods,
            "new_free_good_interval": params.new_free_good_interval,
            "work_week_adjustments": params.work_week_adjustments,
            "work_week_adjustment_interval": params.work_week_adjustment_interval,
            "desired_work_hour_ratio": params.desired_work_hour_ratio
        }

        # create and start the collection thread
        args = self._get_args_from_settings()

        cli_cmd = bin_path + " "
        for arg in args:
            cli_cmd += arg
            cli_cmd += " "

        logger.info(f"Running binary with command: \n   {cli_cmd}")

        self.collector = Collector(bin_path, args)
        self.collector.start_sim_and_begin_collection()

        # internal stuff
        self.t = [0]
        self.current_t = 0
        self.current_week = 0
        self.current_day = 0
        self.stdout_done = False
        self.stderr_done = False
        self.unknown_clients = set()
        self.unknown_labels = set()
        self.current_cout = []

        self.firm_client_types = {"Producer", "Distributor"}
        self.client_dic_lookup = {
            "Simulation": {
                "random_seed": self.record_sim_seed
            },
            "Society": {
                "A": self.record_A_matrix_entry,
                "l": self.record_living_labor_coeff,
                "b": self.record_real_wage_entry,
                "price": self.record_initial_price,
                "new_price": self.record_price_change,
                "mean_consumption_frequency": self.record_consumption_frequency,
                "employment": self.record_current_employment_level,
                "societal_busyness": self.record_societal_busyness,
                "fic": self.record_fic,
                "public_sector_distribution_value": self.record_public_sector_distribution_value,
                "all_distribution_value": self.record_average_consumer_goods_value,
                "public_fund": self.record_public_fund,
                "public_expenditure": self.record_public_expenditure,
                "public_revenue": self.record_public_revenue,
                "work_hours_weekly": self.record_work_hours_weekly,
            },
            "Person": {
                "age": self.record_person_age,
                "account": self.record_person_account,
                "health_status": self.record_person_health,
                "consumption": self.record_person_consumption,
                "ability": self.record_person_ability,
                "inventory": self.record_person_inventory,
                "purchase": self.record_person_purchase,
                "hours_worked": self.record_hours_worked
            },
            "Firm": {
                "input_inventory_level": self.record_input_inventory_level,
                "input_inventory_reduction": self.record_input_inventory_reduction,
                "catalog_addition": self.record_catalog_addition,
                "pursued_plan": self.record_pursued_plan,
                "ended_plan": self.record_plan_ended,
                "shipment_received": self.record_shipment_received,
                "current_demand": self.record_demand_signal,
                "resupply_rate": self.record_resupply_rate,
                "drafting_failure": self.record_drafting_failure,
                "stalled_plan": self.record_stalled_plan,
                "unstalled_plan": self.record_stall_resolved,
                "start_plan_stalled": self.record_start_plan_stalled,
                "start_plan_stall_resolved": self.record_start_plan_stall_resolved,
                "reorder_attempt": self.record_reorder_attempt,
                "reorder_failure": self.record_reorder_failure,
                "newly_employed": self.record_newly_employed,
                "busyness": self.record_sector_busyness,
                "accepted_order": self.record_accepted_order,
                "transfer_request": self.record_transfer_request,
                "transfer": self.record_employment_transfer,
                "account": self.record_firm_account
            },
            "Producer": {},
            "Distributor": {}
        }

        self.traj_update_lookup = {
            "initial": {
                "A": {
                    "function": self.report_A,
                    "keys": ("A",)
                },
                "seed": {
                    "function": self.report_seed,
                    "keys": ("seed",)
                },
            },
            "hourly": {
                "prices": {
                    "function": self.report_current_prices,
                    "keys": (
                        "producer_goods_prices",
                        "consumption_goods_prices",
                        "machine_prices",
                        "producer_goods_values",
                        "consumption_goods_values",
                        "machine_values"
                    )
                },
                "living_labor_amounts": {
                    "function": self.report_current_living_labor_amounts,
                    "keys": (
                        "producer_goods_living_labor_vals",
                        "consumption_goods_living_labor_vals",
                        "machine_prices_living_labor_vals",
                        "l_goods",
                        "l_c_goods",
                        "l_machines"
                    )
                },
                "pre_revolution_real_wage": {
                    "function": self.report_pre_revolution_real_wage,
                    "keys": ("b",)
                },
                "supply": {
                    "function": self.report_firm_supply,
                    "keys": (
                        "producer_supply",
                        "producer_supply_machines",
                        "consumer_goods_supply",
                        "distributor_unshelved_supply"
                    )
                },
                "personal_endowments": {
                    "function": self.report_person_supply,
                    "keys": ("avg_endowments",)
                },
                "accounts": {
                    "function": self.report_accounts,
                    "keys": (
                        "avg_account",
                        "min_account",
                        "max_account",
                        "sectoral_avg_account"
                    )
                },
                "plans_in_progress": {
                    "function": self.report_plans_in_progress,
                    "keys": (
                        "plans_in_progress_goods",
                        "plans_in_progress_c_goods",
                        "plans_in_progress_machines"
                    )
                },
                "goods_in_production": {
                    "function": self.report_goods_in_production,
                    "keys": (
                        "goods_in_production_goods",
                        "goods_in_production_c_goods",
                        "goods_in_production_machines"
                    )
                },
                "person_health": {
                    "function": self.report_person_health,
                    "keys": (
                        "n_healthy",
                        "n_unhealthy"
                    )
                },
                "proficiencies": {
                    "function": self.report_person_skills,
                    "keys": ("average_proficiencies",)
                },
                "consumption_conditions": {
                    "function": self.report_consumption_conditions,
                    "keys": (
                        "mean_consumption_frequencies",
                        "mean_consumption_periods"
                    )
                },
                "pending_inventories": {
                    "function": self.report_pending_inventories,
                    "keys": (
                        "total_pending_inventories_goods",
                        "average_pending_inventories_producers_goods",
                        "average_pending_inventories_distributors_goods",
                        "average_pending_inventories_c_goods",
                        "average_machine_pending_inventory"
                    )
                },
                "reorder_thresholds": {
                    "function": self.report_reorder_thresholds,
                    "keys": (
                        "total_reorder_thresholds_goods",
                        "average_reorder_thresholds_producers_goods",
                        "average_reorder_thresholds_distributors_goods",
                        "average_reorder_thresholds_c_goods",
                        "average_reorder_thresholds_machines"
                    )
                },
                "demands": {
                    "function": self.report_demands,
                    "keys": (
                        "average_demand_producers_goods",
                        "average_demand_distributors_goods",
                        "average_demand_distributors_c_goods",
                        "average_machine_demand"
                    )
                },
                "resupply_rates": {
                    "function": self.report_resupply_rates,
                    "keys": (
                        "resupply_rates_producers_goods",
                        "resupply_rates_distributors_goods",
                        "resupply_rates_machines",
                        "resupply_rates_c_goods"
                    )
                },
                "resupply_deficits": {
                    "function": self.report_resupply_deficits,
                    "keys": (
                        "resupply_deficits_producers_goods",
                        "resupply_deficits_distributors_goods",
                        "resupply_deficits_machines",
                        "resupply_deficits_c_goods"
                    )
                },
                "reorder_attempts": {
                    "function": self.report_reorder_attempts,
                    "keys": (
                        "reorder_attempts_goods",
                        "reorder_attempts_c_goods",
                        "reorder_attempts_machines"
                    )
                },
                "reorder_failures": {
                    "function": self.report_reorder_failures,
                    "keys": (
                        "reorder_failures_goods",
                        "reorder_failures_c_goods",
                        "reorder_failures_machines"
                    )
                },
                "draft_failure_casualties": {
                    "function": self.report_drafting_failure_casualties,
                    "keys": (
                        "hrly_sectoral_drafting_casualty_totals",
                        "goods_drafting_casualties",
                        "c_goods_drafting_casualties",
                        "machines_drafting_casualties"
                    )
                },
                "draft_failure_causes": {
                    "function": self.report_drafting_failure_causes,
                    "keys": (
                        "goods_drafting_failures_caused_by_workers",
                        "c_goods_drafting_failures_caused_by_workers",
                        "machines_drafting_failures_caused_by_workers",
                        "drafting_failures_caused_by_goods",
                        "drafting_failures_caused_by_machines"
                    )
                },
                "employment": {
                    "function": self.report_employment,
                    "keys": (
                        "available_employment_by_sector",
                        "long_run_employment_by_sector",
                        "surplus_labor_ratios",
                        "eqb_employment",
                        "employment",
                        "transfer_requests_by_sector",
                        "transfer_requests_by_sector_t"
                    )
                },
                "busyness": {
                    "function": self.report_busyness,
                    "keys": (
                        "sectoral_busyness",
                        "overall_busyness",
                        "busy_lower_bound",
                        "busy_upper_bound",
                        "work_hours_daily",
                    )
                },
                "activity_levels": {
                    "function": self.report_activity_levels,
                    "keys": (
                        "min_hrly_output",
                        "long_run_activity"
                    )
                },
                "public_accounts": {
                    "function": self.report_public_sector_accounting,
                    "keys": (
                        "fic",
                        "average_consumer_goods_value",
                        "public_fund",
                        "public_revenue",
                        "public_expenditure",
                        "average_public_sector_consumer_goods_value"
                    )
                },
                "ongoing_stall_casualties": {
                    "function": self.report_stalled_plan_casualties,
                    "keys": (
                        "stalled_plan_casualties_goods",
                        "stalled_plan_casualties_c_goods",
                        "stalled_plan_casualties_machines"
                    )
                },
                "ongoing_stall_causes": {
                    "function": self.report_stalled_plan_causes,
                    "keys": (
                        "stalled_plan_causes_goods",
                        "stalled_plan_causes_machines",
                        "stalled_plan_causes_deficits_goods",
                        "stalled_plan_causes_deficits_machines"
                    )
                },
                "start_plan_stall_casualties": {
                    "function": self.report_start_plan_stall_casualties,
                    "keys": (
                        "start_plan_stall_casualties_goods",
                        "start_plan_stall_casualties_machines"
                    )
                },
                "start_plan_stall_causes": {
                    "function": self.report_start_plan_stall_causes,
                    "keys": (
                        "start_plan_stall_causes_goods",
                        "start_plan_stall_causes_machines",
                        "start_plan_stall_causes_deficits_goods",
                        "start_plan_stall_causes_deficits_machines"
                    )
                },
                "firm_accounts": {
                    "function": self.report_firm_accounts,
                    "keys": (
                        "producer_accounts",
                        "distributor_accounts"
                    )
                },
            },
            "daily": {
                "avg_resupply_rates": {
                    "function": self.report_avg_resupply_rates,
                    "keys": (
                        "resupply_rates_producers_goods_daily",
                        "resupply_rates_distributors_goods_daily",
                        "resupply_rates_machines_daily",
                        "resupply_rates_c_goods_daily",
                    )
                },
                "day_counter": {
                    "function": lambda: (Append(self.current_t),),
                    "keys": ("day_counter",)
                }
            },
            "weekly": {
                "order_sizes": {
                    "function": self.report_order_sizes,
                    "keys": (
                        "order_sizes_goods",
                        "order_sizes_c_goods",
                        "order_sizes_machines"
                    )
                },
                "lead_times": {
                    "function": self.report_lead_times,
                    "keys": (
                        "lead_times_goods",
                        "lead_times_c_goods",
                        "lead_times_machines"
                    )
                },
                "team_sizes": {
                    "function": self.report_team_sizes,
                    "keys": (
                        "team_sizes_goods",
                        "team_sizes_c_goods",
                        "team_sizes_machines"
                    )
                },
                "activity_levels": {
                    "function": self.report_weekly_activity_levels,
                    "keys": (
                        "weekly_sectoral_activity_levels",
                    )
                },
                "week_counter": {
                    "function": lambda: (Append(self.current_t),),
                    "keys": ("week_counter",)
                }
            }
        }

        self.persons = {i: {
            "age": 17,
            "account": 0,
            "endowment": np.zeros(self.settings["n_products"]),
            "abilities": np.zeros(self.settings["n_abilities"]),
            "health": "Healthy", # everyone starts in good health
            "recent_busyness": 0.0,
            "employer_id": None,
        } for i in range(self.settings["n_persons"])}

        self.producers = {i: {
            "employees": 0,
            "inventory": np.zeros(self.settings["n_products"]),
            "pending_inventory": np.zeros(self.settings["n_products"]),
            "demand_signals": np.zeros(self.settings["n_products"]),
            "reorder_thresholds": np.zeros(self.settings["n_products"]),
            "resupply_rates": np.zeros(self.settings["n_products"]),
            "resupply_rates_weekly": np.zeros(self.settings["n_products"]),
            "resupply_deficits": np.zeros(self.settings["n_products"]),
            "tracked_inputs": np.zeros(self.settings["n_products"], dtype= bool),
            "recent_labor_hours": 0.0,
            "catalog": [],
            "recent_busyness": 0,
            "inc_inventory": np.zeros(self.settings["n_products"]),
            "account": 0
        } for i in range(self.settings["n_producers"])}

        self.distributors = {i: {
            "employees": 0,
            "inventory": np.zeros(self.settings["n_products"]),
            "pending_inventory": np.zeros(self.settings["n_products"]),
            "demand_signals": np.zeros(self.settings["n_products"]),
            "reorder_thresholds": np.zeros(self.settings["n_products"]),
            "resupply_rates": np.zeros(self.settings["n_products"]),
            "resupply_rates_weekly": np.zeros(self.settings["n_products"]),
            "resupply_deficits": np.zeros(self.settings["n_products"]),
            "tracked_inputs": np.zeros(self.settings["n_products"], dtype= bool),
            "recent_labor_hours": 0.0,
            "catalog": [],
            "recent_busyness": 0,
            "inc_inventory": np.zeros(self.settings["n_products"]),
            "account": 0

        } for i in range(self.settings["n_distributors"])}

        self.active_plans = {i: {"plans": 0, "quantity": 0} for i in range(self.settings["n_products"])}

        self.fic = 0.0
        self.average_consumer_goods_value = 0.0
        self.average_public_sector_consumer_goods_value = 0.0
        self.public_fund = 0.0
        self.public_expenditure = 0.0
        self.public_revenue = 0.0
        self.current_employment = 0
        self.overall_busyness = 0
        self.overall_weekly_busyness = 0
        self.weekly_working_hours = 5*8

        self.A = np.zeros((self.settings["n_products"], self.settings["n_products"]))
        self.l = np.zeros(self.settings["n_products"])
        self.b = np.zeros(self.settings["n_products"])
        self.consumption_frequencies = np.zeros(self.settings["n_products"])
        self.consumption_periods = np.zeros(self.settings["n_products"])

        self.prices = np.zeros(self.settings["n_products"])
        self.living_labor_values = np.zeros(self.settings["n_products"])
        self.old_order_size_avgs = np.zeros(self.settings["n_products"])
        self.old_lead_time_avgs = np.zeros(self.settings["n_products"])
        self.old_team_size_avgs = np.zeros(self.settings["n_products"])
        self.transfer_requests_by_sector_t = np.array([])
        self.long_run_employment_by_sector = np.zeros(self.settings["n_goods"]+self.settings["n_machines"]+1)
        self.long_run_sector_activity = np.zeros(self.settings["n_sectors"])


        self.censored_busyness_mean = None
        self.uncensored_busyness_mean = None
        self.censored_busyness_stddev = None
        self.uncensored_busyness_stddev = None
        self.work_hours_update_this_step = False
        self.individual_busyness_data = []

        self.stalled_plan_casualties = {
            product_id: set() 
            for product_id in range(self.settings["n_products"])
        }
        self.stalled_plan_causes = {
            product_id: {}
            for product_id in range(self.settings["n_products"])
        }
        self.start_plan_stall_casualties = {
            product_id: set()
            for product_id in range(self.settings["n_products"])
        }
        self.start_plan_stall_causes = {
            product_id: {}
            for product_id in range(self.settings["n_products"])
        }

        self.temporary_data = {
            "initial": {},
            "hourly": {
                "transfer_requests_by_sector": 
                    lambda: np.zeros(self.settings["n_sectors"]),
                "reorder_attempts": 
                    lambda: np.zeros(self.settings["n_products"]),
                "reorder_failures": 
                    lambda: np.zeros(self.settings["n_products"]),
                "drafting_failures_casualties_from_workers": 
                    lambda: np.zeros(self.settings["n_products"]),
                "drafting_failures_casualties": 
                    lambda: np.zeros(self.settings["n_products"]),
                "drafting_failures_causes_resources": 
                    lambda: np.zeros(self.settings["n_products"]) 
            },
            "daily": {
                "resupply_rate_data": 
                    self.reset_resupply_rate_data 
            },
            "weekly": {
                "order_sizes": 
                    lambda: [[] for _ in range(self.settings["n_products"])],
                "lead_times": 
                    lambda: [[] for _ in range(self.settings["n_products"])],
                "team_sizes":
                    lambda: [[] for _ in range(self.settings["n_products"])],
                "weekly_quantities_in_production":
                    lambda: np.zeros(self.settings["n_sectors"]),
            }
        }
        for interval in self.temporary_data:
            self.reset_temporary_data(interval)

    def _process_dic(self, dic):
        """ 
        Looks at the contents of a json logged dictionary and updates 
        the relevant quantities accordingly
        """
        label = dic.get("label", "")

        if label == "text_log":
            self.log_text(dic)
            return

        client = dic.get("client", "")

        if client not in self.client_dic_lookup:
            first_time_client = (client not in self.unknown_clients)
            if first_time_client:
                logger.warning(f"Unknown client type: {client}.")
            self.unknown_clients.add(client)
            return

        is_firm = (client in self.firm_client_types)
        if is_firm and label in self.client_dic_lookup["Firm"]:
            record_func = self.client_dic_lookup["Firm"][label]
        else:
            record_func = self.client_dic_lookup[client].get(label)

        if record_func is None:
            first_time_label = ((client, label) not in self.unknown_labels)
            if first_time_label:
                logger.warning(
                    f"Label {label} from client type {client} has no matching recording function"
                )
            self.unknown_labels.add((client, label))
            return
    
        try:
            record_func(dic)
        except Exception as e:
            logger.error(f"Exception while processing label {label} for client {client}: {e}", exc_info= e)
            raise

    def log_text(self, dic):
        extra = {
            "client": dic.get("client", "Unknown"),
            "time": self.current_t,
        }
        msg = "\n"
        for key, value in dic.items():
            if key in ("id", "client", "level", "t", "label"):
                continue

            msg += f"   {key}: {value}\n"
            
        level = dic.get("level", 20)
        logger.log(level, msg, extra= extra)

    def _update_stats(self, interval= "hourly"):
        hourly_dicts = self.traj_update_lookup[interval]
        for group_dict in hourly_dicts.values():
            func = group_dict["function"]
            keys = group_dict["keys"]
            outputs = func()
            if outputs is None:
                continue
            for i in range(len(keys)):
                if outputs[i] is not None:
                    self.traj[keys[i]] = outputs[i]

        self.reset_temporary_data(interval= interval)

    def initialize_properties(self):
        N = self.settings["n_persons"]
        net_weekly_demand = N*24*7*self.consumption_frequencies
        gross_weekly_demand = inv(np.eye(self.settings["n_products"]) - self.A)@net_weekly_demand

        sectoral_weekly_labor_req_raw = self.l * gross_weekly_demand
        min_hrly_output = gross_weekly_demand / (24*7)

        goods_lo, goods_hi = self.get_goods_idxs()
        c_goods_lo, c_goods_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        prod_goods_sectoral_weekly_labor_req = list(sectoral_weekly_labor_req_raw[goods_lo:goods_hi])
        machine_sectoral_weekly_labor_req = list(sectoral_weekly_labor_req_raw[m_lo:m_hi])
        
        prod_goods_min_hrly_output = list(min_hrly_output[goods_lo:goods_hi])
        machines_min_hrly_output = list(min_hrly_output[m_lo:m_hi])

        overall_sectoral_weekly_labor_req = []
        overall_sectoral_activity_levels = []

        overall_sectoral_weekly_labor_req.extend(prod_goods_sectoral_weekly_labor_req)
        overall_sectoral_weekly_labor_req.append(sum(sectoral_weekly_labor_req_raw[c_goods_lo:c_goods_hi]))
        overall_sectoral_weekly_labor_req.extend(machine_sectoral_weekly_labor_req)
        overall_sectoral_weekly_labor_req = np.asarray(overall_sectoral_weekly_labor_req)

        overall_sectoral_activity_levels.extend(prod_goods_min_hrly_output)
        overall_sectoral_activity_levels.append(sum(min_hrly_output[c_goods_lo:c_goods_hi]))
        overall_sectoral_activity_levels.extend(machines_min_hrly_output)
        overall_sectoral_activity_levels = np.asarray(overall_sectoral_activity_levels)

        init_work_hours = self.settings["init_working_day"]
        init_work_days = self.settings["init_working_week"]

        self.predicted_order_sizes = 0.25 * 1.5 * 24*7 * min_hrly_output
        self.eqb_min_employment = overall_sectoral_weekly_labor_req / (init_work_hours*init_work_days)
        self.busy_lower_bd = self.settings["consump_epsilon"]*(init_work_hours*init_work_days / (24*7))
        self.busy_upper_bd = (init_work_hours*init_work_days / (24*7))
        self.min_hrly_output = overall_sectoral_activity_levels
        dim = self.A.shape[0]
        self.values = inv(np.eye(dim) - self.A.T)@self.l

        logger.info(f"A = \n{np.array2string(
            self.A,
            formatter={"float_kind": lambda x: f"{x:10.7f}"}
        )}")

        (evals, evecs) = eig(self.A)
        idx = np.argmax(evals.real)
        r_hat = np.real(evals[idx])

        logger.info(f"Spectral radius of A: {r_hat}")

        if self.settings["init_prices"] == "values":
            self.b = self.consumption_frequencies

        if self.settings["init_prices"] == "equilibrium_prices":
            M = self.A + np.linalg.outer(self.b, self.l)
            (evals, evecs) = eig(M.T)
            idx = np.argmax(evals.real)
            r_hat = np.real(evals[idx])
            epr = 1/r_hat - 1

    def step(self) -> bool:
        """ 
        The basic step function. Retrieves json dictionaries for processing until the logged time step changes or the simulation finishes.
        """
        while True:
            if self.stdout_done and self.stderr_done:
                return True

            item = self.collector.get_next()

            if item.kind == "error":
                raise RuntimeError(f"{item.stream} reader failed: {item.payload}")

            if item.stream == "meta":
                if item.kind == "wait":
                    return False
                continue

            if item.stream == "stderr":
                if item.kind == "eof":
                    self.stderr_done = True
                    if len(self.current_cout) > 0:
                        self._log_cout_and_clear()
                else:
                    logger.info(f"Standard Error: {item.payload}")

            if item.stream == "stdout":
                if item.kind == "eof":
                    self.stdout_done = True
                    if len(self.current_cout) > 0:
                        self._log_cout_and_clear()

                if item.kind == "json":
                    if len(self.current_cout) > 0:
                        self._log_cout_and_clear()

                    dic = item.payload
                    if self.current_t != dic["t"]:
                        self.traj = {}
                        if self.current_t == 0:
                            self.initialize_properties()
                            self._update_stats("initial")
                            self._update_stats("hourly")
                        else:
                            self._update_stats("hourly")

                        self.current_t = dic["t"]
                        if self.current_t % 24 == 0:
                            self.current_day += 1
                            self._update_stats("daily")
                        if self.current_t % (24*7) == 0:
                            self.current_week += 1
                            self._update_stats("weekly")

                        self.traj["t"] = Append(self.current_t)
                        self.t.append(self.current_t)
                        self._process_dic(dic)
                        return False
                    else:
                        self._process_dic(dic)
                else:
                    if item.payload is not None:
                        self.current_cout.append(item.payload)
                continue

            if self.stdout_done and self.stderr_done:
                return True

    def get_data(self):
        if hasattr(self, "traj"):
            return self.traj
        else:
            return {}

    def _log_cout_and_clear(self):
        msg = "Standard Output: \n   "
        for entry in self.current_cout:
            msg += entry
            msg += "\n   "

        logger.info(msg)
        self.current_cout.clear()

    def _get_args_from_settings(self):
        args = [
            "-j",
            "-n", str(self.settings["n_time_steps"]),
            "-p", str(self.settings["n_persons"]),
            "-h", str(self.settings["init_working_day"]),
            "-w", str(self.settings["init_working_week"]),
            "-g", str(self.settings["n_goods"]),
            "-m", str(self.settings["n_machines"]),
            "-r", str(self.settings["n_producers"]),
            "-d", str(self.settings["n_distributors"]),
            "-s", str(self.settings["daily_sick_chance"]),
            "-a", str(self.settings["n_abilities"]),
            "-v", str(self.settings["person_ability_stddev"]),
            "-i", str(self.settings["max_num_inputs_per_good"]),
            "--production_difficulty", str(self.settings["productivity"]),
            "--consumption_demand", str(self.settings["consump_epsilon"]),
            "--init_prices", str(self.settings["init_prices"]),
            "--desired_work_hour_ratio", str(self.settings["desired_work_hour_ratio"]),
        ]

        logger.info(f"\n   {self.settings["fixed_seed"]=}, \n   {self.settings["seed"]=}")
        if self.settings["fixed_seed"]:
            args.append("-e")
            args.append(str(self.settings["seed"]))

        if self.settings["free_goods"]:
            args.append("--public_sector_expansion_period")
            args.append(str(self.settings["new_free_good_interval"]))
        else:
            args.append("--public_sector_expansion_period")
            args.append("0")

        if self.settings["work_week_adjustments"]:
            args.append("--work_week_adjustment_period")
            args.append(str(self.settings["work_week_adjustment_interval"]))
        else:
            args.append("--work_week_adjustment_period")
            args.append("0")

        return args

    def _get_theoretical_values(self, A, l):
        n = A.shape[0]
        vals = inv(np.eye(n) - A.T)@l

        return vals

    def _get_product_property_aggregate(
            self,
            *group_dicts,
            key= "demand_signals",
            operation= "avg",
            tracked_only= True
    ):
        n_products = self.settings["n_products"]
        aggregates = np.zeros(n_products)
        for idx in range(n_products):
            all_vals = []
            for group_dict in group_dicts:
                for member_dict in group_dict.values():
                    if (
                        "tracked_inputs" not in member_dict
                        or tracked_only == False
                        or member_dict["tracked_inputs"][idx]
                    ):
                        all_vals.append(member_dict[key][idx])
            if operation == "avg":
                aggregates[idx] = np.average(all_vals) if all_vals else 0
            elif operation == "sum":
                aggregates[idx] = np.sum(all_vals) if all_vals else 0

        return aggregates

    def get_goods_idxs(self):
        low = 0
        hi = self.settings["n_goods"]
        return low, hi

    def get_consumer_goods_idxs(self):
        low = self.settings["n_goods"]
        hi = 2*self.settings["n_goods"]
        return low, hi

    def get_machines_idxs(self):
        low = 2*self.settings["n_goods"]
        hi = 2*self.settings["n_goods"] + self.settings["n_machines"]
        return low, hi

    def get_sector_idx(self, prod_id):
        n_goods = self.settings["n_goods"]
        n_machines = self.settings["n_machines"]

        if prod_id < n_goods:
            return prod_id

        if prod_id < 2 * n_goods:
            return n_goods  # distribution sector

        machine_idx = prod_id - 2 * n_goods
        return n_goods + 1 + machine_idx

    def _get_average_endowments(self, persons):
        n_prod_goods = self.settings["n_goods"]
        idx_low = n_prod_goods
        idx_high = 2*n_prod_goods
        consumer_goods_idxs = list(range(idx_low, idx_high))

        n_goods = self.settings["n_goods"]

        endowments = [dic["endowment"] for _,dic in persons.items()]
        itemwise_endowments = [[] for i in range(n_prod_goods)]
        for i, idx in enumerate(consumer_goods_idxs):
            for j in range(len(persons)):
                itemwise_endowments[i].append(endowments[j][idx])

        average_endowments = [np.average(itemwise_endowments[i]) for i in range(n_goods)]
        return average_endowments

    def _get_sectoral_drafting_failures_caused_by_workes(self):
        n_sectors = self.settings["n_sectors"]
        sectoral_drafting_failure_totals = np.zeros(n_sectors)
        
    def _get_activity_levels_by_sector(self):
        n_sectors = self.settings["n_sectors"]
        sectoral_activity_levels = np.zeros(n_sectors)


    def _get_good_type_and_idx(self, id):
        """ 
        Return what the index of a good would be 
        if it were in an array only containing goods of the same type 
        """
        n_produced_goods = self.settings["n_goods"]
        if id >= 2*n_produced_goods:
            # machine
            return "machine", id-2*n_produced_goods
        elif id >= n_produced_goods:
            # consumer good
            return "consumer_good", id-n_produced_goods
        else:
            return "production_good", id

    def _get_dist_key(self, dist_id):
        return dist_id - self.settings["n_producers"]

    def _get_group_dict_and_id(self, dic):
        client = dic.get("client")
        if client is None or (client != "Producer" and client != "Distributor"):
            return {}, -1

        raw_id = dic["id"]
        group_dict = self.producers if client == "Producer" else self.distributors
        real_id = raw_id if client == "Producer" else self._get_dist_key(raw_id)

        return group_dict, real_id

    def record_sim_seed(self, dic):
        self.seed = dic["value"]

    def record_A_matrix_entry(self, dic):
        coords_str = dic["coords"]
        coords = tuple(int(num) for num in coords_str[1:len(coords_str)-1].split(","))
        i, j = coords
        a_ij = dic["value"]
        self.A[i][j] = a_ij

    def record_living_labor_coeff(self, dic):
        i = dic["prod_id"]
        l_i = dic["value"]

        self.l[i] = l_i

    def record_real_wage_entry(self, dic):
        i = dic["prod_id"]
        b_i = dic["value"]
        self.b[i] = b_i

    def record_initial_price(self, dic):
        id = dic["product_id"]
        val = dic["price_per_unit"]
        self.prices[id] = val

    def record_price_change(self, dic):
        prod_id = dic["product_id"]
        living_labor_per_unit = dic["living_labor_per_unit"]
        price = dic["price"]
        self.prices[prod_id] = price
        self.living_labor_values[prod_id] = living_labor_per_unit

    def record_consumption_frequency(self, dic):
        id = dic["product_id"]
        val = dic["value"]
        self.consumption_frequencies[id] = val
        self.consumption_periods[id] = 1/max(val, 1e-5)

    def record_current_employment_level(self, dic):
        self.current_employment = dic["total"]

    def record_societal_busyness(self, dic):
        self.overall_busyness = dic["value"]

    def record_fic(self, dic):
        self.fic = dic["value"]

    def record_public_sector_distribution_value(self, dic):
        self.average_public_sector_consumer_goods_value = dic["value"]

    def record_average_consumer_goods_value(self, dic):
        self.average_consumer_goods_value = dic["value"]

    def record_public_fund(self, dic):
        self.public_fund = dic["value"]

    def record_public_expenditure(self, dic):
        self.public_expenditure = dic["value"]

    def record_public_revenue(self, dic):
        self.public_revenue = dic["value"]

    def record_censored_busyness_dist(self, dic):
        self.work_hours_update_this_step = True
        self.censored_busyness_mean = dic["mean"]
        self.censored_busyness_stddev = dic["stddev"]

    def record_uncensored_busyness_dist(self, dic):
        self.work_hours_update_this_step = True
        self.uncensored_busyness_mean = dic["mean"]
        self.uncensored_busyness_stddev = dic["stddev"]

    def record_work_hours_weekly(self, dic):
        self.weekly_working_hours = dic["work_hours_daily"] * dic["work_days_weekly"]
        self.work_hours_update_this_step = True
        work_hours_daily = dic["work_hours_daily"]
        work_days_weekly = dic["work_days_weekly"]
        self.busy_upper_bd = (work_hours_daily * work_days_weekly) / (24*7)

    def record_busyness_data(self, dic):
        self.individual_busyness_data.append(dic["value"])

    def record_person_age(self, dic):
        pass

    def record_person_account(self, dic):
        id = dic["id"]
        account = dic["value"]
        self.persons[id]["account"] = account

    def record_person_health(self, dic):
        id = dic["id"]
        self.persons[id]["health"] = dic["status"]

    def record_person_consumption(self, dic):
        id = dic["id"]
        prod_id = dic["product_id"]
        amt = dic["quantity"]
        self.persons[id]["endowment"][prod_id] -= amt

    def record_person_ability(self, dic):
        ability_id = dic["ability"]
        val = dic["value"]
        person_id = dic['id']
        self.persons[person_id]["abilities"][ability_id] = val

    def record_person_inventory(self, dic):
        prod_id = dic["product_id"]
        amt = dic["amount"]
        person_id = dic['id']
        self.persons[person_id]["endowment"][prod_id] = amt

    def record_person_purchase(self, dic):
        prod_id = dic["product_id"]
        amt = dic["quantity"]
        id = dic["id"]
        self.persons[id]["endowment"][prod_id] += amt

    def record_hours_worked(self, dic):
        pass

    def record_input_inventory_level(self, dic):
        prod_id= dic["product_id"]
        firm_id = dic["id"]
        client = dic["client"]
        amt = dic["amount"]

        if client == "Producer":
            self.producers[firm_id]["inventory"][prod_id] = amt

        if client == "Distributor":
            self.distributors[self._get_dist_key(firm_id)]["inventory"][prod_id] = amt

    def record_input_inventory_reduction(self, dic):
        prod_id = dic["product_id"]
        amt = dic["amount"]
        client = dic["client"]
        firm_id = dic["id"]

        if client == "Producer":
            self.producers[firm_id]["inventory"][prod_id] -= amt
 
        if client == "Distributor":
            self.distributors[self._get_dist_key(firm_id)]["inventory"][prod_id] -= amt

    def record_shipment_received(self, dic):
        prod_id = dic["product_id"]
        client = dic["client"]
        amt = dic["amount"]
        firm_id = dic["id"]

        if client == "Producer":
            self.producers[firm_id]["inc_inventory"][prod_id] -= amt

        if client == "Distributor":
            dist_id = self._get_dist_key(firm_id)
            self.distributors[dist_id]["inc_inventory"][prod_id] -= amt

    def record_reorder_attempt(self, dic):
        prod_id = dic["product_id"]
        self.reorder_attempts[prod_id] += 1

    def record_catalog_addition(self, dic):
        product_id = dic["product_id"]
        client = dic["client"]
        firm_id = dic["id"]

        if client == "Producer":
            self.producers[firm_id]["catalog"].append(product_id)

        if client == "Distributor":
            self.distributors[self._get_dist_key(firm_id)]["catalog"].append(product_id)

    def record_accepted_order(self, dic):
        pass

    def _is_distributor(self, firm_id):
        return (firm_id >= self.settings["n_producers"])

    def record_pursued_plan(self, dic):
        customer_id = dic["customer_id"]
        is_distributor = self._is_distributor(customer_id)
        if is_distributor:
            customer_id = self._get_dist_key(customer_id)
        prod_id = dic["product_id"]
        sector_id = self.get_sector_idx(prod_id)
        quantity = dic["quantity"]
        lead_time = dic.get("lead_time", 0)
        team_size = dic.get("num_workers")

        self.active_plans[prod_id]["plans"] += 1
        self.active_plans[prod_id]["quantity"] += quantity
        self.lead_times[prod_id].append(lead_time)
        self.order_sizes[prod_id].append(quantity)
        self.long_run_sector_activity[sector_id] += quantity
        self.weekly_quantities_in_production[sector_id] += quantity

        if team_size is not None:
            self.team_sizes[prod_id].append(team_size)

        if is_distributor:
            self.distributors[customer_id]["inc_inventory"][prod_id] += quantity
        else:
            self.producers[customer_id]["inc_inventory"][prod_id] += quantity

    def record_plan_ended(self, dic):
        prod_id = dic["product_id"]
        amt = dic["quantity"]
        self.active_plans[prod_id]["plans"] -= 1
        self.active_plans[prod_id]["quantity"] -= amt

    def record_demand_signal(self, dic):
        prod_id = dic["product_id"]
        group_dict, real_id = self._get_group_dict_and_id(dic)
        if real_id == -1:
            return

        group_dict[real_id]["tracked_inputs"][prod_id] = True
        group_dict[real_id]["demand_signals"][prod_id] = dic["demand"]
        group_dict[real_id]["reorder_thresholds"][prod_id] = dic["reorder_threshold"]

    def record_resupply_rate(self, dic):
        prod_id = dic["product_id"]
        client = dic["client"]
        group_dict, real_id = self._get_group_dict_and_id(dic)
        if real_id == -1:
            return

        prod_id = dic["product_id"]
        group_dict[real_id]["resupply_rates"][prod_id] = dic["resupply_rate"]
        self.resupply_rate_data[client][prod_id].append(dic["resupply_rate"])
        group_dict[real_id]["resupply_deficits"][prod_id] = dic["resupply_deficit"]

    def record_stalled_plan(self, dic):
        plan_id = dic["plan_id"]
        product_id = dic["product_id"]
        missing_resource_id = dic["missing_resource"]
        deficit = dic["deficit"]
        self.stalled_plan_casualties[product_id].add(plan_id)
        self.stalled_plan_causes[missing_resource_id][plan_id] = deficit

    def record_stall_resolved(self, dic):
        plan_id = dic["plan_id"]
        product_id = dic["product_id"]

        self.stalled_plan_casualties[product_id].discard(plan_id)

        for plan_deficits in self.stalled_plan_causes.values():
            plan_deficits.pop(plan_id, None)

    def get_plan_stall_deficits(self):
        return [
            sum(plan_deficits.values())
            for plan_deficits in self.stalled_plan_causes.values()
        ]

    def record_start_plan_stalled(self, dic):
        plan_id = dic["plan_id"]
        product_id = dic["product_id"]
        missing_resource_id = dic["missing_resource"]
        deficit = dic["deficit"]

        self.start_plan_stall_casualties[product_id].add(plan_id)
        self.start_plan_stall_causes[missing_resource_id][plan_id] = deficit


    def record_start_plan_stall_resolved(self, dic):
        plan_id = dic["plan_id"]
        product_id = dic["product_id"]

        self.start_plan_stall_casualties[product_id].discard(plan_id)

        for plan_deficits in self.start_plan_stall_causes.values():
            plan_deficits.pop(plan_id, None)

    def record_drafting_failure(self, dic):
        casualty_id = dic["product_id"]
        reason = dic["reason"]
        if reason == "insufficient_workers":
            self.drafting_failures_casualties_from_workers[casualty_id] += 1
            self.drafting_failures_casualties[casualty_id] += 1
        elif reason == "insufficient_resources":
            self.drafting_failures_casualties[casualty_id] += 1
            missing_products_str_list = dic["missing_products"].split(",")
            missing_product_ids = [
                int(prod) for prod in missing_products_str_list if prod
            ]
            for product_id in missing_product_ids:
                self.drafting_failures_causes_resources[product_id] += 1

    def record_reorder_failure(self, dic):
        prod_id = dic["product_id"]
        self.reorder_failures[prod_id] += 1

    def record_newly_employed(self, dic):
        firm_id = dic["id"]
        worker_id = dic["worker_id"]
        self.persons[worker_id]["employer_id"] = firm_id
        client = dic["client"]
        if client == "Producer": 
            self.producers[firm_id]["employees"] += 1

        if client == "Distributor":
            dist_id = self._get_dist_key(firm_id)
            self.distributors[dist_id]["employees"] += 1

    def record_sector_busyness(self, dic):
        firm_id = dic["id"]
        client = dic["client"]
        firm_busyness = dic["firm_busyness"]
        recent_labor_hours = dic.get("recent_labor_hours",0)

        if client == "Producer": 
            self.producers[firm_id]["recent_busyness"] = firm_busyness
            self.producers[firm_id]["recent_labor_hours"] = recent_labor_hours

        if client == "Distributor":
            dist_id = self._get_dist_key(firm_id)
            self.distributors[dist_id]["recent_busyness"] = firm_busyness
            self.distributors[dist_id]["recent_labor_hours"] = recent_labor_hours

    def record_employment_transfer(self, dic):
        old_emp = dic["old_workplace_id"]
        worker_id = dic["worker_id"]
        old_emp_is_distributor = (old_emp >= self.settings["n_producers"])
        if old_emp_is_distributor:
            old_emp = self._get_dist_key(old_emp)
            self.distributors[old_emp]["employees"] -= 1
        else:
            self.producers[old_emp]["employees"] -= 1

        new_emp = dic["new_workplace_id"]
        self.persons[worker_id]["employer_id"] = new_emp
        new_emp_is_distributor = (new_emp >= self.settings["n_producers"])
        if new_emp_is_distributor:
            new_emp = self._get_dist_key(new_emp)
            self.distributors[new_emp]["employees"] += 1
        else:
            self.producers[new_emp]["employees"] += 1

    def record_transfer_request(self, dic):
        firm_id = dic["id"]
        client = dic["client"]
        if client == "Producer":
            cat = self.producers[firm_id]["catalog"]
        elif client == "Distributor":
            dist_id = self._get_dist_key(firm_id)
            cat = self.distributors[dist_id]["catalog"]
        else:
            return

        if len(cat) == 0:
            return

        if client == "Producer":
            for product_id in cat:
                sector_idx = self.get_sector_idx(product_id)
                self.transfer_requests_by_sector[sector_idx] += 1
        else:
            sector_idx = self.get_sector_idx(cat[0])
            self.transfer_requests_by_sector[sector_idx] +=  1

        if (
            len(self.transfer_requests_by_sector_t) == 0 
            or self.current_t != self.transfer_requests_by_sector_t[-1]
        ):
            self.transfer_requests_by_sector_t = np.append(
                self.transfer_requests_by_sector_t,
                self.current_t
            )

    def record_firm_account(self, dic):
        client = dic["client"]
        firm_id = dic["id"]
        value = dic["value"]
        if client == "Producer":
            self.producers[firm_id]["account"] = value
        elif client == "Distributor":
            firm_id = self._get_dist_key(firm_id)
            self.distributors[firm_id]["account"] = value
        else:
            return

    def report_A(self):
        return (Replace(self.A),)

    def report_seed(self):
        if not hasattr(self, "seed"):
            return

        return (
            Update(details= {"param": "seed", "value": int(self.seed)}),
        )

    def report_current_prices(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        return (
            Append(self.prices[good_lo:good_hi]),
            Append(self.prices[c_good_lo:c_good_hi]),
            Append(self.prices[m_lo:m_hi]),
            Append(self.values[good_lo:good_hi]),
            Append(self.values[c_good_lo:c_good_hi]),
            Append(self.values[m_lo:m_hi]),
        )


    def report_current_living_labor_amounts(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        l_list = list(self.l)

        return (
            Append(self.living_labor_values[good_lo:good_hi]),
            Append(self.living_labor_values[c_good_lo:c_good_hi]),
            Append(self.living_labor_values[m_lo:m_hi]),
            Append(l_list[good_lo:good_hi]),
            Append(l_list[c_good_lo:c_good_hi]),
            Append(l_list[m_lo:m_hi]),
        )

    def report_pre_revolution_real_wage(self):
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()

        return (
            Append(self.b[c_good_lo:c_good_hi]),
        )

    def report_firm_supply(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        producer_supply = self._get_product_property_aggregate(
            self.producers,
            key= "inventory",
            operation= "sum",
            tracked_only= False
        )
        distributor_supply = self._get_product_property_aggregate(
            self.distributors,
            key= "inventory",
            operation= "sum",
            tracked_only= False
        )

        return (
            Append(producer_supply[good_lo:good_hi]),
            Append(producer_supply[m_lo:m_hi]),
            Append(distributor_supply[c_good_lo:c_good_hi]),
            Append(distributor_supply[good_lo:good_hi]),
        )

    def report_person_supply(self):
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        inventories = self._get_product_property_aggregate(
            self.persons, key= "endowment"
        )

        return (Append(inventories[c_good_lo:c_good_hi]),)

    def _get_sectoral_avg_accounts(self):
        sectoral_accounts = [[] for _ in range(self.settings["n_sectors"])]
        for person_dict in self.persons.values():
            account = person_dict["account"]
            employer_id = person_dict["employer_id"]
            if employer_id is not None:
                if self._is_distributor(employer_id):
                    dist_dict = self.distributors[self._get_dist_key(employer_id)]
                    catalog = dist_dict["catalog"]
                else:
                    prod_dict = self.producers[employer_id]
                    catalog = prod_dict["catalog"]
                for product_id in catalog:
                    sector_id = self.get_sector_idx(product_id)
                    sectoral_accounts[sector_id].append(account)

        return [np.average(accounts) for accounts in sectoral_accounts]

    def report_accounts(self):
        accounts = [dic["account"] for _, dic in self.persons.items()]
        sectoral_avg_accounts = self._get_sectoral_avg_accounts()

        return (
            Append(np.average(accounts)),
            Append(np.min(accounts)),
            Append(np.max(accounts)),
            Append(sectoral_avg_accounts),
        )

    def report_plans_in_progress(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        plans_in_motion_goods = [
            self.active_plans[i]["plans"] for i in range(good_lo,good_hi)
        ]
        plans_in_motion_c_goods = [
            self.active_plans[i]["plans"] for i in range(c_good_lo,c_good_hi)
        ]
        plans_in_motion_machines = [
            self.active_plans[i]["plans"] for i in range(m_lo,m_hi)
        ]

        return (
            Append(plans_in_motion_goods),
            Append(plans_in_motion_c_goods),
            Append(plans_in_motion_machines),
        )

    def report_goods_in_production(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        quantities_in_prod_goods = [
            self.active_plans[i]["quantity"] for i in range(good_lo,good_hi)
        ]
        quantities_in_prod_c_goods = [
            self.active_plans[i]["quantity"] for i in range(c_good_lo,c_good_hi)
        ]
        quantities_in_prod_machines = [
            self.active_plans[i]["quantity"] for i in range(m_lo,m_hi)
        ]

        return (
            Append(quantities_in_prod_goods),
            Append(quantities_in_prod_c_goods),
            Append(quantities_in_prod_machines),
        )

    def report_person_health(self):
        health_statuses = [
            0 if dic["health"] == "Healthy" 
            else 1 for _, dic in self.persons.items()
        ]
        n_unhealthy = sum(health_statuses)
        n_healthy = len(self.persons) - n_unhealthy

        return (
            Append(n_healthy),
            Append(n_unhealthy)
        )

    def _get_average_abilities(self, persons):
        n_abilities = self.settings["n_abilities"]

        abilities = [dic["abilities"] for _,dic in persons.items()]

        abilitywise_profs = [[] for _ in range(n_abilities)]
        for i in range(n_abilities):
            for j in range(len(persons)):
                abilitywise_profs[i].append(abilities[j][i])

        average_proficiencies = [
            np.average(abilitywise_profs[i]) for i in range(n_abilities)
        ]
        return average_proficiencies

    def report_person_skills(self):
        average_proficiencies = self._get_average_abilities(self.persons)
        return (Append(average_proficiencies),)

    def report_consumption_conditions(self):
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()

        return (
            Append(self.consumption_frequencies[c_good_lo:c_good_hi]),
            Append(self.consumption_periods[c_good_lo:c_good_hi]),
        )

    def _set_pending_inventories(self):
        for _, producer_dict in self.producers.items():
            producer_dict["pending_inventory"] = producer_dict["inventory"]+producer_dict["inc_inventory"]

        for _, distributor_dict in self.distributors.items():
            distributor_dict["pending_inventory"] = distributor_dict["inventory"]+distributor_dict["inc_inventory"]

    def report_pending_inventories(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        self._set_pending_inventories()
        average_pending_inventories_producers = self._get_product_property_aggregate(
            self.producers, key="pending_inventory"
        )
        average_pending_inventories_distributors = self._get_product_property_aggregate(
            self.distributors, key="pending_inventory"
        )
        overall_pending_inventory = self._get_product_property_aggregate(
            self.producers, self.distributors,
            key= "pending_inventory",
        )

        return (
            Append(overall_pending_inventory[good_lo:good_hi]),
            Append(average_pending_inventories_producers[good_lo:good_hi]),
            Append(average_pending_inventories_distributors[good_lo:good_hi]),
            Append(average_pending_inventories_distributors[c_good_lo:c_good_hi]),
            Append(average_pending_inventories_producers[m_lo:m_hi]),
        )

    def report_reorder_thresholds(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        average_reorder_thresholds_producers = self._get_product_property_aggregate(
            self.producers, key="reorder_thresholds"
        )
        average_reorder_thresholds_distributors = self._get_product_property_aggregate(
            self.distributors, key="reorder_thresholds"
        )
        overall_reorder_thresholds = self._get_product_property_aggregate(
            self.producers, self.distributors,
            key= "reorder_thresholds", 
        )

        return (
            Append(overall_reorder_thresholds[good_lo:good_hi]),
            Append(average_reorder_thresholds_producers[good_lo:good_hi]),
            Append(average_reorder_thresholds_distributors[good_lo:good_hi]),
            Append(average_reorder_thresholds_distributors[c_good_lo:c_good_hi]),
            Append(average_reorder_thresholds_producers[m_lo:m_hi]),
        )

    def report_demands(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        average_demands_producers = self._get_product_property_aggregate(
            self.producers, key="demand_signals"
        )
        average_demands_distributors = self._get_product_property_aggregate(
            self.distributors, key="demand_signals"
        )

        return (
            Append(average_demands_producers[good_lo:good_hi]),
            Append(average_demands_distributors[good_lo:good_hi]),
            Append(average_demands_distributors[c_good_lo:c_good_hi]),
            Append(average_demands_producers[m_lo:m_hi]),
        )

    def report_resupply_rates(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        average_resupply_rates_producers = self._get_product_property_aggregate(
            self.producers, key="resupply_rates"
        )
        average_resupply_rates_distributors = self._get_product_property_aggregate(
            self.distributors, key="resupply_rates"
        )

        return (
            Append(average_resupply_rates_producers[good_lo:good_hi]),
            Append(average_resupply_rates_distributors[good_lo:good_hi]),
            Append(average_resupply_rates_producers[m_lo:m_hi]),
            Append(average_resupply_rates_distributors[c_good_lo:c_good_hi]),
        )

    def report_resupply_deficits(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        average_resupply_deficits_producers = self._get_product_property_aggregate(
            self.producers, key="resupply_deficits"
        )
        average_resupply_deficits_distributors = self._get_product_property_aggregate(
            self.distributors, key="resupply_deficits"
        )

        return (
            Append(average_resupply_deficits_producers[good_lo:good_hi]),
            Append(average_resupply_deficits_distributors[good_lo:good_hi]),
            Append(average_resupply_deficits_producers[m_lo:m_hi]),
            Append(average_resupply_deficits_distributors[c_good_lo:c_good_hi]),
        )

    def report_reorder_attempts(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        return (
            Append(self.reorder_attempts[good_lo:good_hi]),
            Append(self.reorder_attempts[c_good_lo:c_good_hi]),
            Append(self.reorder_attempts[m_lo:m_hi]),
        )

    def report_reorder_failures(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        return (
            Append(self.reorder_failures[good_lo:good_hi]),
            Append(self.reorder_failures[c_good_lo:c_good_hi]),
            Append(self.reorder_failures[m_lo:m_hi]),
        )

    def report_drafting_failure_casualties(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        drafting_casualty_totals_goods = list(
            self.drafting_failures_casualties[good_lo:good_hi]
        )
        drafting_casualty_totals_distribution = [
            np.sum(self.drafting_failures_casualties[c_good_lo:c_good_hi])
        ]
        drafting_casualty_totals_machines = list(
            self.drafting_failures_casualties[m_lo:m_hi]
        )
        
        sectoral_drafting_casualty_totals = drafting_casualty_totals_goods + \
                                            drafting_casualty_totals_distribution + \
                                            drafting_casualty_totals_machines

        return (
            Append(sectoral_drafting_casualty_totals),
            Append(self.drafting_failures_casualties[good_lo:good_hi]),
            Append(self.drafting_failures_casualties[c_good_lo:c_good_hi]),
            Append(self.drafting_failures_casualties[m_lo:m_hi]),
        )

    def report_drafting_failure_causes(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        return (
            Append(self.drafting_failures_casualties_from_workers[good_lo:good_hi]),
            Append(self.drafting_failures_casualties_from_workers[c_good_lo:c_good_hi]),
            Append(self.drafting_failures_casualties_from_workers[m_lo:m_hi]),
            Append(self.drafting_failures_causes_resources[good_lo:good_hi]),
            Append(self.drafting_failures_causes_resources[m_lo:m_hi]),
        )


    def _get_available_employment_by_sector(self):
        n_sectors = self.settings["n_sectors"]
        sectoral_employment = np.zeros(n_sectors)
        for properties in self.producers.values():
            cat = properties["catalog"]
            employees = properties["employees"]
            for prod_id in cat:
                sector_id = self.get_sector_idx(prod_id)
                sectoral_employment[sector_id] += employees

        for properties in self.distributors.values():
            sectoral_employment[self.settings["n_goods"]] += properties["employees"]

        return sectoral_employment

    def report_employment(self):
        sectoral_employment = self._get_available_employment_by_sector()
        self.long_run_employment_by_sector += sectoral_employment

        surplus_labor_ratios = self.eqb_min_employment / sectoral_employment

        return (
            Append(sectoral_employment),
            Append(
                self.long_run_employment_by_sector / max(self.current_t, 1)
            ),
            Append(surplus_labor_ratios),
            Append(self.eqb_min_employment),
            Append(self.current_employment),
            Append(self.transfer_requests_by_sector),
            Replace(self.transfer_requests_by_sector_t)
        )

    def _get_sectoral_busyness(self):
        n_sectors = self.settings["n_goods"]+self.settings["n_machines"]+1

        sectoral_busyness_data = [[] for _ in range(n_sectors)]
        for properties in self.producers.values():
            cat = properties["catalog"]
            busyness = properties["recent_busyness"]
            for prod_id in cat:
                sector_id = self.get_sector_idx(prod_id)
                sectoral_busyness_data[sector_id].append(busyness)

        for properties in self.distributors.values():
            sectoral_busyness_data[self.settings["n_goods"]].append(properties["recent_busyness"])

        sectoral_busyness = np.array([np.average(sector) if sector else 0.0 for sector in sectoral_busyness_data])
        return sectoral_busyness

    def report_busyness(self):
        sectoral_busyness = self._get_sectoral_busyness()
        return(
            Append(sectoral_busyness),
            Append(self.overall_busyness),
            Append(self.busy_lower_bd),
            Append(self.busy_upper_bd),
            Append(self.weekly_working_hours / self.settings["init_working_week"]),
        )

    def report_activity_levels(self):
        return (
            Append(self.min_hrly_output),
            Append(self.long_run_sector_activity / max(self.current_t, 1)),
        )

    def report_public_sector_accounting(self):
        return (
            Append(self.fic),
            Append(self.average_consumer_goods_value),
            Append(self.public_fund),
            Append(self.public_revenue),
            Append(self.public_expenditure),
            Append(self.average_public_sector_consumer_goods_value)
        )

    def report_stalled_plan_casualties(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        stalled_plan_casualties_goods = [
            len(self.stalled_plan_casualties[i]) for i in range(good_lo,good_hi)
        ]
        stalled_plan_casualties_c_goods = [
            len(self.stalled_plan_casualties[i]) for i in range(c_good_lo,c_good_hi)
        ]
        stalled_plan_casualties_machines = [
            len(self.stalled_plan_casualties[i]) for i in range(m_lo,m_hi)
        ]

        return (
            Append(stalled_plan_casualties_goods),
            Append(stalled_plan_casualties_c_goods),
            Append(stalled_plan_casualties_machines)
        )

    def report_stalled_plan_causes(self):
        good_lo, good_hi = self.get_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        stalled_plan_causes_goods = [len(self.stalled_plan_causes[i]) for i in range(good_lo,good_hi)]
        stalled_plan_causes_machines = [len(self.stalled_plan_causes[i]) for i in range(m_lo,m_hi)]

        stalled_plan_causes_total_deficits = self.get_plan_stall_deficits()
        stalled_plan_causes_total_deficits_goods = stalled_plan_causes_total_deficits[good_lo:good_hi]
        stalled_plan_causes_total_deficits_machines = stalled_plan_causes_total_deficits[m_lo:m_hi]

        return (
            Append(stalled_plan_causes_goods),
            Append(stalled_plan_causes_machines),
            Append(stalled_plan_causes_total_deficits_goods),
            Append(stalled_plan_causes_total_deficits_machines),
        )

    def report_start_plan_stall_casualties(self):
        good_lo, good_hi = self.get_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        start_plan_stall_casualties_goods = [
            len(self.start_plan_stall_casualties[i])
            for i in range(good_lo, good_hi)
        ]
        start_plan_stall_casualties_machines = [
            len(self.start_plan_stall_casualties[i])
            for i in range(m_lo, m_hi)
        ]

        return (
            Append(start_plan_stall_casualties_goods),
            Append(start_plan_stall_casualties_machines)
        )

    def report_start_plan_stall_causes(self):
        good_lo, good_hi = self.get_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        start_plan_stall_causes_goods = [
            len(self.start_plan_stall_causes[i])
            for i in range(good_lo, good_hi)
        ]
        start_plan_stall_causes_machines = [
            len(self.start_plan_stall_causes[i])
            for i in range(m_lo, m_hi)
        ]
        start_plan_stall_deficits = [
            sum(plan_deficits.values())
            for plan_deficits in self.start_plan_stall_causes.values()
        ]
        start_plan_stall_deficits_goods = \
            start_plan_stall_deficits[good_lo:good_hi]
        start_plan_stall_deficits_machines = \
            start_plan_stall_deficits[m_lo:m_hi]

        return (
            Append(start_plan_stall_causes_goods),
            Append(start_plan_stall_causes_machines),
            Append(start_plan_stall_deficits_goods),
            Append(start_plan_stall_deficits_machines),
        )


    def report_avg_resupply_rates(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        resupply_rate_data_producers = self.resupply_rate_data["Producer"]
        average_resupply_rates_producers_daily = [
            np.average(prod_data) if len(prod_data) > 0 else 0
            for prod_data in resupply_rate_data_producers
        ]

        resupply_rate_data_distributors = self.resupply_rate_data["Distributor"]
        average_resupply_rates_distributors_daily = [
            np.average(prod_data) if len(prod_data) > 0 else 0
            for prod_data in resupply_rate_data_distributors
        ]

        return (
            Append(average_resupply_rates_producers_daily[good_lo:good_hi]),
            Append(average_resupply_rates_distributors_daily[good_lo:good_hi]),
            Append(average_resupply_rates_producers_daily[m_lo:m_hi]),
            Append(average_resupply_rates_distributors_daily[c_good_lo:c_good_hi])
        )

    def report_order_sizes(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        order_size_averages = []
        for i, order_size_data in enumerate(self.order_sizes):
            if len(order_size_data) > 0:
                order_size_averages.append(np.average(order_size_data))
            else:
                order_size_averages.append(self.old_order_size_avgs[i])

        self.old_order_size_avgs = order_size_averages

        return (
            Append(order_size_averages[good_lo:good_hi]),
            Append(order_size_averages[c_good_lo:c_good_hi]),
            Append(order_size_averages[m_lo:m_hi])
        )

    def report_lead_times(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        lead_time_averages = []
        for i, lead_size_data in enumerate(self.lead_times):
            if len(lead_size_data) > 0:
                lead_time_averages.append(np.average(lead_size_data))
            else:
                lead_time_averages.append(self.old_lead_time_avgs[i])

        self.old_lead_time_avgs = lead_time_averages

        return (
            Append(lead_time_averages[good_lo:good_hi]),
            Append(lead_time_averages[c_good_lo:c_good_hi]),
            Append(lead_time_averages[m_lo:m_hi])
        )

    def report_team_sizes(self):
        good_lo, good_hi = self.get_goods_idxs()
        c_good_lo, c_good_hi = self.get_consumer_goods_idxs()
        m_lo, m_hi = self.get_machines_idxs()

        team_size_averages = []
        for i, team_size_data in enumerate(self.team_sizes):
            if len(team_size_data) > 0:
                team_size_averages.append(np.average(team_size_data))
            else:
                team_size_averages.append(self.old_team_size_avgs[i])

        self.old_team_size_avgs = team_size_averages

        return (
            Append(team_size_averages[good_lo:good_hi]),
            Append(team_size_averages[c_good_lo:c_good_hi]),
            Append(team_size_averages[m_lo:m_hi])
        )

    def report_weekly_activity_levels(self):
        return (
            Append(self.weekly_quantities_in_production / (24*7)),
        )

    def report_firm_accounts(self):
        return (
            Append(np.array([value["account"] for value in self.producers.values()])),
            Append(np.array([value["account"] for value in self.distributors.values()]))
        )

    def reset_resupply_rate_data(self):
        return {
            "Producer": [[] for _ in range(self.settings["n_products"])],
            "Distributor": [[] for _ in range(self.settings["n_products"])]
        }

    def reset_temporary_data(self, interval= "hourly"):
        interval_dict = self.temporary_data[interval]
        for name, reset_func in interval_dict.items():
            setattr(self, name, reset_func())
