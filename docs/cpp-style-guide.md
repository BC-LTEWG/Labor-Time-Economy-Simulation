> **Note — delete every Note before the push.**
> Where the code and Google differ there are **two rules with the same id**:
> **a** = as the code is today, **b** = Google C++ Style Guide. Delete the one you do not want (the letter can stay).
> Notes give the numbers on `main` today (`header/` + `src/`, 30 files, 4,176 lines, without Eigen and doctest)
> and the cost of each choice. When done, force-push the branch: that is the final guide.
> Fact for every choice: `main` does not follow the wiki today either.

# Base
- **B1a** This guide is the whole standard. Anything it does not cover is the reviewer's call.
- **B1b** We follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html). This guide adds to it and, where the two differ, this guide wins.

> **Note:** B1b alone changes no code: the a/b pairs below decide each conflict. B1b only answers "what about the
> cases nobody wrote down" with a standard every new student can look up.

# Files
- **F1** Header files. File names end in `h`. If the file contains a class definition, it should contain only one principal class definition, but may contain closely-related structure type or class definitions. All such files should be placed in the top-level directory `header`.
- **F2a** The name of the file is the name of the principal type defined therein, which should be in UpperCamelCase: `class Person` is defined in `Person.h` and `Person.cpp`. If a header file contains other global data, not a type definition, its name should be simple and in lower case: `constants.h` for symbolic constant definitions.
- **F2b** File names are all lowercase, with `_` between words: `class PriceController` is defined in `price_controller.h` and `price_controller.cc`.
- **F3** Implementation (source code) files. Every type definition header file should have a corresponding source code file, with the same name (F2). All functions should be defined out of line, and the out-of-line definitions should be placed in the appropriate source code files. All source code files should be placed in the `src` top-level directory. `main` contains the `main` function and belongs in the `src` directory.
- **F4** Makefile. A project-level makefile should appear as the top level and should be named `makefile`. Subdirectories may contain makefiles temporarily for development purposes, but they must eventually be removed.

> **Note F2:** today 29 of 30 files are `UpperCamelCase` (only `main.cpp` is not); the constants file is `Constants.h`,
> not `constants.h` as the wiki says.
> Cost of **b**: rename 29 files, every `#include`, the makefile. On Windows a rename that only changes case needs two
> `git mv` steps. No behavior change.
>
> **Note F3:** Google allows short functions (about 10 lines) in the header; F3 is stricter, which Google permits. No conflict.

# Code within a File
- **C1** Avoid long lines. Limit lines to 80 characters. Use `:set cc=80` to have Vi draw a vertical line at column 80. (Use `:set cc=` to make the vertical line disappear.)
- **C2a** Avoid comments. Use self-explanatory names and clear logic instead.
- **C2b** Every non-obvious class or struct has a comment saying what it is for. Almost every function declaration has a comment saying what it does; leave it out only when the function is simple and obvious. Comments explain why, not what.
- **C3** Avoid long functions. If you feel the need for comments or vertical spaces within a function, your function is probably too long and complicated. Each function should "do only one thing and do it well."

> **Note C1:** same as Google. Today 83 lines are longer than 80.
>
> **Note C2:** today there are 12 comment lines in all of `header/` + `src/`.
> Cost of **b**: about 20 classes and 195 function declarations to judge, comments written by hand.
> If **b**: the Code Review bullet on comments in `git-workflow.md` changes too, and C3's first sentence goes.

# Order Within a File
The following sections must be separated from each other by single vertical spaces.
- **O1a** `#pragma once` declaration (header only).
- **O1b** `#define` guard (header only), named after the path: `#ifndef LTE_HEADER_PERSON_H_` / `#define LTE_HEADER_PERSON_H_` / `#endif`.
- **O2a** `#include` directives for system (C++ library) headers, in alphabetical order. Include only the headers that the compiler needs for the symbols in the current file! Then `#include` directives for project headers, in alphabetical order.
- **O2b** `#include` directives in this order, each group alphabetical: the file's own header (`Person.cpp` includes `Person.h` first), C system headers, C++ standard library headers, other libraries' headers (`Eigen`), project headers. Include only the headers that the compiler needs for the symbols in the current file!
- **O3** Symbolic constants defined through the `#define` directive. These should be avoided, except (perhaps) for those used for conditional compilation. Symbolic constants, in general, should be global "variables" qualified as `const` and defined in `header/constants.h`. Include only the necessary headers!
- **O4** Type definitions. Accessory type definitions (such as structure type definitions of types used in class definitions) should come first.
  - Within a class definition, the `public` section comes before the `private` section. The `public` section should contain public (API) functions only, with constructors and destructor coming first:
    - Default constructor (if any)
    - Non-default constructors (if any)
    - Copy constructor (if any)
    - Destructor (if any)
    - Other API functions (if any).
  - Enumeration types, wherever possible, should be associated with specific classes and should be defined within their respective class definitions. Typically, an enumeration type belongs in the `public` section.
  - The `private` section should list functions above fields.
  - Structure types should not use access specifiers at all — they should only be used for cases where everything is `public` (by default), and should be used for types that serve primarily as mere data containers.
- **O5** Function definitions. Overloaded functions should occur in sequence, with overloads using smaller (or empty) parameter lists coming first.

> **Note O1:** today `#pragma once` in 16 of 16 headers. Google forbids it. Cost of **b**: 16 headers, 3 lines each.
>
> **Note O2:** today system headers first, then project headers (O2a). Cost of **b**: automatic (clang-format).
> Pro of **b**: a header that cannot compile on its own fails at once. `Order.h` and `PriceController.h`
> do not compile on their own (found while removing dead code, #200).
>
> **Note O4:** Google's class order is the same idea (public first, functions above data). No conflict.

# Symbol Names
- **N7** Custom-defined types (class, structure, union, and enumeration types) are named in UpperCamelCase: `Person`, `PublicSector`.
- **N8a** Functions (other than constructors and destructors) are in lower\_snake\_case: `compute_fic()`.
- **N8b** Functions (other than constructors and destructors) are in UpperCamelCase: `ComputeFic()`. Accessors and mutators may be in lower\_snake\_case, like variables: `balance_hours()`, `set_balance_hours()`.
- **N9** Global and local variables (including function parameters), and local constants (but not global constants) are in lower\_snake\_case.
- **N10a** Data members are in lower\_snake\_case, with no prefix or suffix: `balance_hours`.
- **N10b** Data members are in lower\_snake\_case. Data members of classes (not structs) end with `_`: `balance_hours_`.
- **N11a** Global constants (defined in `constants.h`) are in ALL\_CAPS\_SNAKE\_CASE: `WORK_WEEK_MAX_HOURS`.
- **N11b** Global constants (defined in `constants.h`) are `k` + UpperCamelCase: `kWorkWeekMaxHours`.
- **N12a** Enumerators in enumeration types are in ALL\_CAPS\_SNAKE\_CASE: `HEALTHY`.
- **N12b** Enumerators in enumeration types are `k` + UpperCamelCase, like constants: `kHealthy`.

> **Note N8:** today 195 function names, all lower\_snake\_case (59 `get_`, 11 `set_`).
> Cost of **b**: about 125 functions renamed; the names appear about 900 times. Semi-automatic (clang-tidy rename).
> If **b**: the verb forms N70–N82 below become `GetThing()`, `ComputeThing()`… except plain accessors.
>
> **Note N10:** the wiki does not say; today no member ends with `_` (0 of about 109).
> Cost of **b**: about 109 members renamed, up to 1,700 uses. Pro of **b**: a member and a parameter can share a name.
>
> **Note N11:** today about 67 ALL\_CAPS constants. Cost of **b**: search and replace, about 220 uses.
>
> **Note N12:** today **mixed**. ALL\_CAPS: `Logger::Client` (8), `Person::HealthStatus` (2).
> `k` form: `Order::OrderStatus` (4) and `Product::ProductType` (4), both by Amittai; `ArgType` in `main.cpp` (19), by Nathan.
> Cost of **a**: 27 enumerators renamed. Cost of **b**: 10 enumerators renamed.
> Pro of **b**: no clash with macros. Today `Logger::ERROR` clashes with the `ERROR` macro of `windows.h`.

# Horizontal Spacing
- **H1a** Four spaces for every indentation level. Do _not_ use tab characters!
- **H1b** Two spaces for every indentation level; `public:` and `private:` are indented one space. Do _not_ use tab characters!
- **H2a** One space before and after the `*` used as part of a type designation in a declaration: `Person * person`. The `&` goes with the type name: `Person& person`.
- **H2b** The `*` and `&` go with the type name: `Person* person`, `Person& person`.
- **H3** One space before and after a binary arithmetic, relational, or logical operator and before and after the assignment operator: `-`, `+`, `*`, `/`, `%`, `==`, `!=`, `<`, `<=`, `>=`, `>`, `&&`, `||`, `<<`, `>>`, `&`, `|`
- **H4** No space around a member access operator: `person.get_age()`, `person->get_age()`
- **H5** No space between a unary operator and its operand: `*deref`, `!value`, `++value`, `value++`, `--value`, `value--`, `~bit_flip`.

> **Note H1 + H2:** today 4 spaces and `Person * person` (186 times; `Person* person` 0 times).
> Both are automatic (clang-format). With **a**, formatting `main` changes 1,408 lines; with **b**, 3,776 of 4,176
> lines (90%). Cost of **b**: `git blame` shows the reformat commit for almost every line, and every open branch
> conflicts once (e.g. #276).
> H2a's `&` rule is from Amittai's review on #89.

# Vertical Spaces
- **V1** _No_ vertical spaces within a function body. If you are tempted to insert a vertical space in a function body, you should probably break the function up into smaller functions.

> **Note V1:** Google says "sparingly"; V1 is stricter, which Google permits. Today 84 blank lines inside functions.

# Language Features
- **L1a** No `auto`, unless the type is very long and complicated. Write the type.
- **L1b** Use `auto` only when it makes the code clearer or safer, not to avoid writing the type.
- **L2a** When a value that the upstream logic must prevent shows up anyway (0 units in a plan, a plan with no workers), throw an exception with a meaningful message. Do not guard against it silently.
- **L2b** Do not use exceptions.
- **L3** Braces always, also around one statement.

> **Note L1:** today 0 `auto`. No cost either way.
>
> **Note L2:** Today 12 `throw`, 3 `try`.
> Cost of **b**: redesign the error handling. Google says its own rule comes from old code that is not
> exception-safe, and would probably be different if it started from scratch.
>
> **Note L3:** same as Google.

# Rule Ids
Every rule has an id, so that a review can cite one: "N21: `h` → `hours_worked`". Letters: B base, F files, C code within a file, O order within a file, N names, H horizontal spacing, V vertical spaces, L language features, D design, W docs, G git (in `git-workflow.md`). When no rule fits, ask. If a name is hard to find, the design is usually wrong (N131).

# Names: Principles
| Id | Rule |
|---|---|
| N1 | A name says **what** it is, in **which unit**, and in **which role**. A reader who sees only the name can guess all three. |
| N2 | Domain words come from the book (GIC) and the glossary below, not from programming jargon. |
| N3 | One concept, one word, everywhere: code, docs, CSV columns, logs, plots. |
| N4 | One word, one concept. If a word has two meanings, one of them gets another word. A unit is not a concept: `hours` is the unit of labor time (`labor_hours`) and of simulated time (`sim_time`). |
| N5 | Same shape of name for the same kind of thing. Once `work_week_max_hours` exists, it is not `max_hours_per_week` elsewhere. |
| N6 | Long and clear beats short and cryptic. The length of a name grows with its scope: a 3-line loop may use `i`; a class member may not. |

# Names: Form
In addition to Symbol Names above.

| Id | Kind | Form | Example |
|---|---|---|---|
| N15 | Macros (avoid them, see above) | `LTE_ALL_CAPS` | `LTE_ASSERT` |
| N16 | Namespaces | short `lowercase` | `lte` |
| N17 | Template parameters | UpperCamelCase noun | `Value`, `Key` |
| N19 | Acronyms are words | `FicRule`, `fic`, `CsvWriter` | not `FICRule`, `CSVWriter` |
| N20 | Spelling | US English, as in the project name: `labor`, not `labour`. Quotes from the book keep their spelling. | |

# Names: Words
| Id | Rule | Bad | Good |
|---|---|---|---|
| N21 | Full words. No abbreviations except the list in N22. | `h`, `qty`, `cnt`, `prc`, `pp`, `cg` | `hours_worked`, `quantity_units`, `worker_count`, `consumer_good` |
| N22a | Allowed abbreviations: `id`, `min`, `max`, `rng` (random generator), `csv`, `json`, `io`, `sim` (simulated), and book terms in the glossary (`fic`). Nothing else without adding it here. | | |
| N22b | Allowed abbreviations: `id`, `min`, `max`, `csv`, `json`, `io`, `sim` (simulated), and book terms in the glossary (`fic`). Nothing else without adding it here. The random generator is `rand_gen`. | | |
| N23 | No vague words: `data`, `info`, `value` (alone), `item`, `object`, `thing`, `stuff`, `temp`, `tmp`, `result` (except the one returned value of a short function), `flag`, `status` (alone), `type` (alone), `misc`. | `plan_data`, `tmp` | `plan_summary`, `unmet_demand_units` |
| N24 | No noise words: `the_`, `my_`, `a_`, `_obj`, `_var`, `_list` (say what is in it), `_struct`. | `the_firm`, `order_list` | `supplier`, `pending_orders` |
| N25 | No type in the name: `dbl_rate`, `p_person`, `vec_orders`, `str_name`. The type is in the declaration. | `vec_workers` | `workers` |
| N26 | Spell out the meaning, not the implementation. | `idle_set` | `idle_workers` |
| N27 | Do not encode a negation. | `not_employed` | `is_employed` used as `!is_employed` |
| N28 | Pairs of opposites are fixed: `add/remove`, `insert/erase`, `begin/end`, `first/last`, `min/max`, `open/close`, `start/stop`, `credit/debit`, `hire/release`, `source/target`, `previous/next`, `before/after`. Never mix (`add`/`delete`, `start`/`end`). | | |

> **Note N21:** Google says the same: no abbreviations a reader outside the project would not know.
>
> **Note N22:** 
> Today `main` uses neither (`gen`, `get_random_generator()`). No cost either way.

# Names: Quantities and Units
| Id | Rule | Example |
|---|---|---|
| N30 | Every number with a unit carries the unit as the **last** part of its name. | `hours_worked`, `fund_balance_hours`, `stock_units` |
| N31 | Mold: `<thing>_<qualifier>_<unit>`. The qualifier (`total`, `mean`, `max`, `min`, `target`, `previous`) goes right before the unit. | `work_week_max_hours`, `consumption_total_hours` |
| N32 | Rates: `<thing>_<unit>_per_<unit>`. | `price_hours_per_unit`, `consumption_units_per_hour` |
| N33 | Units in this project. Labor time and simulated time: `_hours` (one time step = one hour). Calendar spans only if they really are days or weeks: `_days`, `_weeks`. Physical amounts of a product: `_units`. | `labor_hours_per_unit`, `window_hours`, `order_units` |
| N34 | Parts of a whole in 0..1: `_share`. Never percent in code; if a value is 0..100 for display only, `_percent`. | `public_consumption_share` |
| N35 | Factors that are not a share (can exceed 1): `_factor`. Probabilities: `_probability`. | `productivity_factor`, `illness_probability` |
| N36 | Counts: `<things>_count`. Not `num_`, `n_`, `nr_` (`num_sales` vs `sales_num` is ambiguous). | `worker_count` |
| N37 | Positions in a container: `<thing>_index`. Identity: `<thing>_id`. Never mix the three. | `firm_index`, `firm_id`, `firm_count` |
| N38 | Ranges: half-open `[begin, end)` uses `begin/end`; closed `[first, last]` uses `first/last`. | `first_hour`, `last_hour` |
| N39 | A book term keeps its book name and has a glossary entry with its unit (`fic`: share 0..1). The book name wins over N34. | `fic` |
| N40 | When a strong unit type exists (`Hours`, `Units`), the type carries the unit and the suffix may go. Until then, the suffix is required. | `Hours hours_worked` |

> **Note:** Google has no rule on units, maps, ids or the glossary (N30–N56). No conflict.

# Names: Ids, Objects and Roles
| Id | Rule | Bad | Good |
|---|---|---|---|
| N41 | Name a variable after its **role**, not its type. | `Person& person`, `Firm * f` | `Person& worker`, `Firm& supplier`, `Firm& buyer` |
| N42 | A variable holding an id ends with `_id`. A variable holding the object does not. | `PersonId worker` | `PersonId worker_id`; `Person& worker` |
| N43 | Each kind of id is its own type. | `int id` | `PersonId`, `FirmId`, `ProductId` |
| N44 | Parameters say which one when there are two of a kind. | `transfer(Firm&, Firm&)` | `transfer(Firm& source_firm, Firm& target_firm)` |
| N45 | Pointers and references are not marked in the name (`p_`, `_ptr`, `_ref`). Ownership is in the type (`std::unique_ptr`). | `firm_ptr` | `supplier` |

# Names: Collections and Maps
| Id | Rule | Example |
|---|---|---|
| N50 | Sequences and sets: plural of the element, with the role. | `workers`, `idle_workers`, `pending_orders` |
| N51 | Maps: `<value>_by_<key>`. Value singular when there is one per key, plural when there are several. | `price_by_product`, `workers_by_firm` |
| N52 | Maps keyed by id use the entity name, not `_id`, after `by_`. | `employer_by_worker` (key `PersonId`) |
| N53 | Map from a thing to a count or quantity: the value's name with its unit. | `consumption_hours_by_product` |
| N54 | Two keys: a struct key or a named type, not `x_by_y_by_z`. | `PriceTable` with `get_price_hours_per_unit(product, hour)` |
| N55 | No `std::pair`/`std::tuple` in interfaces: a struct with named fields. | `struct Quote { FirmId supplier_id; double price_hours_per_unit; };` |
| N56 | Loop variables over a collection use the singular role. | `for (const Person& worker : workers)` |

# Names: Booleans
| Id | Rule | Bad | Good |
|---|---|---|---|
| N60 | Booleans and predicates read as a yes/no question: `is_`, `has_`, `can_`, `should_`, `was_`. | `employed`, `check_stock()` | `is_employed`, `has_stock()` |
| N61 | Positive form only (N27). | `is_not_full`, `disable_logging` | `has_room`, `is_logging` |
| N62 | A predicate only answers: returns `bool`, changes nothing. | `is_valid()` that also fixes the value | `is_valid()` + `repair()` |
| N63 | No boolean parameters that switch behavior; use an enum or two functions. | `step(true)` | `step(StepMode::WARM_UP)` |

> **Note N63:** with N12b the example becomes `StepMode::kWarmUp`.

# Names: Functions
A function name is a promise. Each verb below has a contract; a function that breaks it gets another name.

| Id | Form | Promise |
|---|---|---|
| N70 | `get_<thing>()` | Cheap read of existing state. No side effects. `get_balance_hours()` |
| N71 | `compute_<thing>()` | Calculates from its inputs, may be costly, no side effects. `compute_fic(consumption)` |
| N72 | `find_<thing>()` | Lookup that may fail: returns `std::optional` or a null pointer, never throws for "not found". |
| N73 | `count_<things>()` | Returns a count. |
| N74 | `is_/has_/can_<...>()` | Predicate (N62). |
| N75 | `make_<thing>()` / `create_<thing>()` | Builds and returns a new object. `make_` for values, `create_` for entities with an id. |
| N76 | `set_<field>()` | Assigns that one field, nothing else. |
| N77 | `to_<type>()` | Conversion that builds a new value. |
| N78 | `load_/save_<thing>()` | File or network I/O. Only at the edge of the program, never in the model core. |
| N79 | `try_<action>()` | May not happen; returns `bool` or `std::optional` saying whether it did. |
| N80 | A verb that says what changes: `credit_work`, `hire`, `release`, `ship`, `record_sale`. Not `update`, `process`, `handle`, `do`, `manage`, `perform`, `execute`. If no specific verb fits, the function does too much. | |
| N81 | `on_<event>()` | Called by someone else when `<event>` happens. `on_hour_end()` |
| N82 | Units are in parameter names: `credit_work(PersonId worker_id, double hours_worked)`. | |

> **Note N70–N82:** written for N8a. With N8b: `GetThing()`, `ComputeFic()`, `FindThing()`… and N70 becomes the
> plain accessor `balance_hours()`. The promises stay the same.

# Names: Types and Modules
| Id | Rule | Example |
|---|---|---|
| N90 | A type is a noun for what it **is**. A module that holds a rule or does one job is a noun for that job. | `CreditLedger`, `LaborAllocator` |
| N91 | Neutral, technical names. Words for institutions (`Office`, `Market`, `Bureau`, `Agency`, `Council`, `Fund`) only when the book names that institution; cite the section. | `LaborAllocator`, not `LabourOffice` |
| N92 | Role suffixes, used only with this meaning: `-Allocator` assigns a resource, `-Coordinator` matches parties, `-Ledger` records balances, `-Store` holds entities by id, `-Rule` is an exchangeable policy (interface), `-Recorder` writes output, `-Generator` creates the starting world. | |
| N93 | Suffixes that say nothing: `Manager`, `Handler`, `Processor`, `Helper`, `Util(s)`, `Service`, `Data`, `Info`, `Object`, `Impl`. | |
| N94 | Interfaces are named after the abstraction, implementations after what makes them different. | `AllocationRule`; `BusynessTransfers`, `RosteredWorkWeek` |
| N95 | If the one-sentence description of a type needs "and", split it (D1). | |

# Names: Time
| Id | Rule | Example |
|---|---|---|
| N100 | Simulated time is `sim_time`, in hours: `sim_time_hours` for a point, `<thing>_hours` for a duration. Labor time is `labor_hours` (N4). | `start_sim_time_hours`, `window_hours` |
| N101 | Values that belong to a period say so. | `hours_worked_this_week`, `hours_worked_previous_week` |
| N102 | `previous_` / `next_` for neighbors in a sequence; never `old`/`last` for that meaning (`last` is N38). | `previous_fic` |
| N103 | No `current_`, `now_`, `new_` when the value is just the value. | `fic`, not `current_fic` |

# Names: Process and Review
| Id | Rule |
|---|---|
| N130 | Naming is a process (Belshee): missing → nonsense → honest → honest and complete → does the right thing → intent → domain abstraction. A long honest name is a valid step; it shows what to split. |
| N131 | When a name needs "and", "or", "then", "maybe", split the thing (D1). |
| N132 | Renames are separate commits from behavior changes. |

Review checklist (cite the id):

1. Can I tell the unit? (N30)
2. Can I tell the role, not just the type? (N41)
3. Id, index or count, and which one? (N37, N42)
4. Map: what is the key, what is the value? (N51)
5. Does the verb keep its promise? (N70–N82)
6. Is the word the glossary's word? (N2, N3)
7. Any abbreviation, vague word or negation? (N21, N23, N27)
8. Would a newcomer guess it from the name alone? (N1)

# Design
| Id | Rule |
|---|---|
| D1 | One job per type and per function. Its one-sentence description has no "and". |
| D2 | One owner per quantity. A number (a price, the labor per unit, the FIC) is stored and written in one place; everyone else reads it. |
| D3 | No mutable global state and no singletons. A function gets what it uses as parameters. Global constants are fine (see above). |
| D4 | Rules from the book are pure functions where possible (`compute_fic(consumption)`), so a test can check them without building a world. |
| D6 | Saved state refers to entities by id, not by pointer. |
| D9 | Every rule of the model names its source: a book section with URL, anchor and access date, or "model choice". Never invent a source; an illustrative example is labeled as an example. |
| D10 | Module names follow N90–N95; adding an institution is a model change, not a refactor. |

> **Note D3:** today `main` has singletons (`Society::get_instance()` 40 calls, `PriceController::get_instance()` 5,
> `Sim::get_instance()` 1), so D3 describes the target, not today.

# Writing Docs
| Id | Rule |
|---|---|
| W3 | A proposal shows **today** and **proposed** side by side. |
| W4 | Few words. Tables and diagrams over paragraphs. Cut every sentence that carries no information. |
| W5 | Citations: book, chapter, section, URL with anchor, access date. Example: GIC ch. VI §3, <https://www.marxists.org/subject/left-wing/gik/1930/06.htm#h3>, accessed 12 Oct 2026. |

# Glossary
Book: GIC, *Fundamental Principles of Communist Production and Distribution* (1930), English translation 1990, <https://www.marxists.org/subject/left-wing/gik/1930/index.htm>. Add an entry when a term enters the code.

| Code name | Meaning | Unit | Book |
|---|---|---|---|
| `sim_time` | Simulated time: one time step = one hour | hours | model choice |
| `labor_hours` | Labor time; L: labor expended | hours | "labour-hours", ch. VI §3 |
| `means_of_production_hours` | P: means of production used up | hours | ch. VI §3 |
| `raw_materials_hours` | C: raw materials used up | hours | ch. VI §3 |
| `public_establishment` | Establishment for general social use, whose output is not sold to individuals | | "GSU (public) establishments", ch. VI §1, §3 |
| qualifier `public` (N31) | The same quantity for public establishments (index u): `labor_public_hours` | | $P_u$, $C_u$, $L_u$, ch. VI §3 |

# Sources
- Google C++ Style Guide, <https://google.github.io/styleguide/cppguide.html>, accessed 17 Oct 2026.
- Naming maps `values_by_key`: Eden Soft Labs, [VD12: Make the meanings of keys and values obvious](https://edensoftlabs.atlassian.net/wiki/spaces/edensoftlabs/pages/103842004/Make+the+Meanings+of+Keys+and+Values+Obvious+in+Map+Dictionary+Lookup+Types+VD12); S. Kaul, [Naming map variables](https://shivankaul.com/blog/naming-maps).
- Units in names: Linux, [Standard unit suffixes for property names](https://codex.ac6.fr/Linux/xref/Linux-v4.19/Documentation/devicetree/bindings/property-units.txt); pandaprosumer, [Unit system and conventions](https://pandaprosumer.readthedocs.io/en/stable/about/units.html).
- Verbs that keep their promise: Arnaoudova, Di Penta, Antoniol, "Linguistic antipatterns: what they are and how developers perceive them", Empirical Software Engineering 2016.
- Qualifiers at the end, `num` ambiguity, opposites: S. McConnell, *Code Complete* 2nd ed., ch. 11.
- Name molds and consistency: F. Hermans, *The Programmer's Brain*, ch. 8; Feitelson et al., [How developers choose names](https://arxiv.org/pdf/2103.07487).
- Naming as a process: A. Belshee, "Good naming is a process, not a single step".
- Booleans, no negatives: OpenStack API SIG, [naming guidelines, boolean fields](https://specs.openstack.org/openstack/api-sig/guidelines/naming.html#boolean-fields).
