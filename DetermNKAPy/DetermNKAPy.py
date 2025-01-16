import re
from collections import defaultdict
import csv
import sys

def read_moore_to_list(positions, file, alphabet):
    symbols_set = set()  
    lines = file.readlines()

    temp_row = lines[0].strip().split(';')
    temp_row_st = lines[1].strip().split(';')
    #состояния
    for i in range(1, len(temp_row)):
        state = {
            "state": temp_row_st[i],
            "output": temp_row[i],
            "transitions": []  
        }
        positions.append(state)

    for line in lines[2:]:
        temp_row = line.strip().split(';')
        input_sym = temp_row[0]

        if input_sym != "Оµ":
           symbols_set.add(input_sym)

        for i in range(1, len(temp_row)):
            transition = {
                "inputSym": input_sym,
                "nextPos": temp_row[i].split(',') if temp_row[i] else [] 
            }
            positions[i - 1]["transitions"].append(transition)

    # Преобразование множества в список
    alphabet = list(symbols_set)

    return positions, alphabet

def export_moore_automata_to_csv(moore_automaton, filename):
    all_input_symbols = set()
    for state in moore_automaton:
        for transition in state['transitions']:
            #if transition['nextPos']: 
            all_input_symbols.add(transition['inputSym'])

    all_input_symbols = sorted(all_input_symbols)

    with open(filename, mode='w', newline='', encoding='utf-8') as file:
        writer = csv.writer(file, delimiter=';')

        output_row = ['']
        for state in moore_automaton:
            output_row.append(state['output'])
        writer.writerow(output_row)

        headers = [''] + [state['state'] for state in moore_automaton]
        writer.writerow(headers)


        for symbol in all_input_symbols:
            row = [symbol]

            for state in moore_automaton:
                next_states = []
                for transition in state['transitions']:
                    if transition['inputSym'] == symbol:
                        next_states.append(transition['nextPos'])

                if not next_states:
                    row.append('')
                else:
                    output_str = ','.join(next_states)  #  все состояния
                    row.append(output_str)

            writer.writerow(row)

    print(f"Moore automaton exported to {filename}")


def epsilon_closure(states, moore_automaton, reachable_states=None):
    if reachable_states is None:
        reachable_states = set()

    next_states = set()
    for state in states:
        if state not in reachable_states:
            reachable_states.add(state)  # Mark state as visited
            for transition in moore_automaton[state]['transitions']:
                if transition['inputSym'] == "ε":  
                    next_states.update(transition['nextPos'])

    if not next_states:
        return reachable_states
    else:
        return epsilon_closure(next_states, moore_automaton, reachable_states)


def convert_nfa_to_dfa(moore_automata, symbols):
    dfa_automaton = [] 
    new_name_state_map = {} # {x0: S0; x1, x2: S2}
    eps_state_map = {} # {x1: x1,x2}
    queue = []
    
    symbols = [symbol for symbol in symbols if symbol != "ε"]
    for state in moore_automata:
        #получаем все состояния с ε переходом
        eps_state_map[state] = frozenset(epsilon_closure({state}, moore_automata))

    start_state = [list(moore_automata.keys())[0]]
    frozStartState = frozenset(start_state)
    new_name_state_map[frozStartState] = "S0"
    queue.append(start_state)
    counter = 0

    while queue:
        print(queue)
        currState = queue.pop(0)
        currStateFrozen = frozenset(currState)
        dfa_stateNew = {
            "state": new_name_state_map[currStateFrozen],
            "output": "",
            "transitions": []
        }
        #объединяем ε-замыкания у всех состояний NFA в DFA
        curr_state = frozenset().union(*(eps_state_map[state] for state in currState))

        for state in curr_state:
            if moore_automata[state]['output'] == "F":
                dfa_stateNew['output'] = "F"

            for symbol in symbols:
                for transition in moore_automata[state]['transitions']:
                    if transition['inputSym'] == symbol:
                        if transition['nextPos']:
                            #список NFA состояний в которые можно перейти из текущего DFA состояния
                            next_positions = list(transition['nextPos'])
                            #ищем переход в DFA c тем же символом 
                            existing_transition = next(
                                (t for t in dfa_stateNew['transitions'] if t['inputSym'] == symbol), None
                            )

                            if existing_transition:
                                #обновляем список, добавляя новые переходы из existing
                                existing_transition['nextPos'].extend(
                                    pos for pos in next_positions if pos not in existing_transition['nextPos']
                                )
                            else:
                                #создаём новый переход и добавляем его в список переходов текущего состояния DFA
                                dfa_stateNew['transitions'].append({
                                    'inputSym': symbol,
                                    'nextPos': next_positions
                                })

        for transition in dfa_stateNew['transitions']:
            frozSet = frozenset(transition['nextPos'])

            if frozSet:
                #проверяем обозначили ли множество ранее
                if frozSet not in new_name_state_map.keys():
                    counter += 1
                    new_name_state_map[frozSet] = f"S{counter}"
                    queue.append(transition['nextPos'])
                #меняем список переходов NFA на DFA переход
                transition['nextPos'] = new_name_state_map[frozSet]

        dfa_automaton.append(dfa_stateNew)
    return dfa_automaton


def main():

    if len(sys.argv) != 3:
        print("Usage: lab3 grammar.txt output.csv")
        sys.exit(1)

    grammar_file = sys.argv[1]
    output_file = sys.argv[2]

    # grammar_file = "source_nfa.csv"
    # output_file = "out.csv"
    positions = []  # Список для состояний
    symbols = []   # Алфавит входных символов


    with open(grammar_file, 'r', encoding='utf-8') as file:
        positions, symbols = read_moore_to_list(positions, file, symbols)

    moore_automata = {}
    #преобразуем состояния в ключ
    for state in positions:
        moore_automata[state["state"]] = {
            "output": state["output"],
            "transitions": state["transitions"]
        }

    dfa_automata = convert_nfa_to_dfa(moore_automata, symbols)

    export_moore_automata_to_csv(dfa_automata, output_file)
if __name__ == "__main__":
    main()
