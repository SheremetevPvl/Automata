import re
from collections import defaultdict
import csv
from sre_constants import LITERAL
import sys
from enum import Enum

class ServiceSymbols(Enum):
    START_IN = "("
    OUT_IN = ")"
    OR = "|"
    EMPTY_SYMB = ""
    EPSILANT = "ε"
    MIN_ONE_ITER = "*"
    MIN_ZERO_ITER = "+"

class SymbolType(Enum):
    LITERAL = 'Literal'
    CONCAT = 'Concat'
    OR = 'Or'
    REPEAT = 'Repeat'
    PLUS = 'Plus'

def export_moore_automaton_to_csv(moore_automaton, filename, start_state):
    # Получаем все уникальные inputSym
    all_input_symbols = set()

    for key in moore_automaton:
        state = moore_automaton[key]
        for transition in state['transitions']:
            #if transition['nextPos']:  # используемые символы ввода
            all_input_symbols.add(transition['inputSym'])

    all_input_symbols = sorted(all_input_symbols)

    state_keys = [start_state] + [key for key in moore_automaton if key != start_state]
    with open(filename, mode='w', newline='', encoding='utf-8') as file:
        writer = csv.writer(file, delimiter=';')

        # Первая строка: выходные значения
        output_row = [''] + [moore_automaton[key]['output'] for key in state_keys]
        writer.writerow(output_row)

        # Вторая строка: имена состояний
        headers = [''] + state_keys
        writer.writerow(headers)

        # Остальные строки: переходы для каждого символа ввода
        for symbol in all_input_symbols:
            row = [symbol]
            for key in state_keys:
                # Ищем переход для данного символа
                state = moore_automaton[key]
                next_states = [
                    transition['nextPos']
                    for transition in state['transitions']
                    if transition['inputSym'] == symbol
                ]

                if not next_states:
                    row.append('')  # Пустая ячейка, если нет переходов
                else:
                    output_str = ','.join(next_states)  # Все состояния через запятую
                    row.append(output_str)

            writer.writerow(row)

    print(f"Moore automaton exported to {filename}")

def parse_regex(pattern):
    def parse_or(expr):
        parts = []
        current_expr = []
        depth = 0  # вложенность скобок
        #получаем первое выражение на самом верхнем уровне
        for char in expr:
            if char == ServiceSymbols.START_IN.value:
                depth += 1
            elif char == ServiceSymbols.OUT_IN.value:
                depth -= 1
            elif char == ServiceSymbols.OR.value and depth == 0:  #только на верхнем уровне
                parts.append("".join(current_expr))
                current_expr = []
                continue
            current_expr.append(char)
        #a|(b|c) -> [a, (b|c)]
        parts.append("".join(current_expr))
        if len(parts) > 1:
            return {"type": "Or", "left": parse_concat(parts[0]), "right": parse_or(ServiceSymbols.OR.value.join(parts[1:]))}
        else:
             return parse_concat(expr)

    def parse_concat(expr):
        parts = []
        i = 0
        #представляем выражение в виде древа: подвыражения перибираем рекурсией, остально в узлы
        while i < len(expr):
            if expr[i] == ServiceSymbols.START_IN.value:
                #ищем закрывающую скобку
                end = find_closing_parenthesis(expr, i)
                #получаем всё, что внутри скобок
                sub_expr = expr[i + 1:end]
                if sub_expr == ServiceSymbols.EMPTY_SYMB.value:  # Если пусто в узел пишем `ε`
                    parts.append({"type": "Literal", "value": ServiceSymbols.EPSILANT.value})
                else:
                    parts.append(parse_or(sub_expr))
                i = end + 1
            elif expr[i] == ServiceSymbols.MIN_ONE_ITER.value or expr[i] == ServiceSymbols.MIN_ZERO_ITER.value:
                if not parts:
                    raise ValueError(f"Unexpected operator '{expr[i]}' at position {i}")
                op = "Repeat" if expr[i] == ServiceSymbols.MIN_ONE_ITER.value else "Plus"
                parts[-1] = {"type": op, "expr": parts[-1]}
                i += 1
            else:
                parts.append({"type": "Literal", "value": expr[i]})
                i += 1

        # Создание Б-дерева для конкатенации
        if len(parts) == 1:
            return parts[0]

        #  Concat(a, Concat(b, c))
        return build_concat_tree(parts)

    def build_concat_tree(parts):
        if len(parts) == 1:
            return parts[0]

        # Рекурсивно строим дерево конкатенации
        left = parts[0]
        right = build_concat_tree(parts[1:])
        return {"type": "Concat", "left": left, "right": right}

    def find_closing_parenthesis(expr, start):
        depth = 1
        for i in range(start + 1, len(expr)):
            if expr[i] == ServiceSymbols.START_IN.value:
                depth += 1
            elif expr[i] == ServiceSymbols.OUT_IN.value:
                depth -= 1
                if depth == 0:
                    return i
        raise ValueError("unclosed expression")

    return parse_or(pattern)

state_counter = 1

def add_eps_trans(nfa, start_state, final_state):
    nfa[start_state]['transitions'].append({
                                    'inputSym': 'ε',
                                    'nextPos': final_state
                                })
    return nfa

def get_new_state():
    global state_counter
    state = {
            "state": f"q{state_counter}",
            "output": '',
            "transitions": []
        }
    state_counter += 1
    return state

def tree_to_nfa(regex, startState, finalState, nfa):
    if regex['type'] == SymbolType.LITERAL.value:
        global state_counter
        start_state = get_new_state()
        final_state = get_new_state()
        start_state["transitions"].append({
                                    'inputSym': regex['value'],
                                    'nextPos': final_state['state']
                                })

        nfa[start_state['state']] = start_state
        nfa[final_state['state']] = final_state

        return nfa, start_state['state'], final_state['state']

    elif regex['type'] == SymbolType.CONCAT.value:

        left_nfa, lstart, lfinal = tree_to_nfa(regex['left'], startState, finalState, nfa)
        right_nfa, rstart, rfinal = tree_to_nfa(regex['right'], startState, finalState, nfa)

        nfa = add_eps_trans(nfa, lfinal, rstart)

        return nfa, lstart, rfinal

    elif regex['type'] == SymbolType.OR.value:

        left_nfa, lstart, lfinal = tree_to_nfa(regex['left'], startState, finalState, nfa)
        right_nfa, rstart, rfinal = tree_to_nfa(regex['right'], startState, finalState, nfa)

        start_state = get_new_state()
        final_state = get_new_state()

        nfa[start_state['state']] = start_state
        nfa[final_state['state']] = final_state

        nfa = add_eps_trans(nfa, start_state['state'], lstart)
        nfa = add_eps_trans(nfa, lfinal, final_state['state'])

        nfa = add_eps_trans(nfa, start_state['state'], rstart)
        nfa = add_eps_trans(nfa, rfinal, final_state['state'])

        return nfa, start_state['state'], final_state['state']

    elif regex['type'] == SymbolType.REPEAT.value:

        sub_nfa, sub_start, sub_final = tree_to_nfa(regex['expr'], startState, finalState, nfa)

        start_state = get_new_state()
        final_state = get_new_state()

        nfa[start_state['state']] = start_state
        nfa[final_state['state']] = final_state

        nfa = add_eps_trans(nfa, start_state['state'], sub_start)
        nfa = add_eps_trans(nfa, sub_final, sub_start)
        nfa = add_eps_trans(nfa, sub_final, final_state['state'])
        nfa = add_eps_trans(nfa, start_state['state'], final_state['state'])

        return nfa, start_state['state'], final_state['state']

    elif regex['type'] == SymbolType.PLUS.value:

        sub_nfa, sub_start, sub_final = tree_to_nfa(regex['expr'], startState, finalState, nfa)

        start_state = get_new_state()
        final_state = get_new_state()

        nfa[start_state['state']] = start_state
        nfa[final_state['state']] = final_state

        nfa = add_eps_trans(nfa, start_state['state'], sub_start)
        nfa = add_eps_trans(nfa, sub_final, sub_start)
        nfa = add_eps_trans(nfa, sub_final, final_state['state'])

        return nfa, start_state['state'], final_state['state']

def main():

    if len(sys.argv) != 3:
        print('Usage: /regexToNFA <output.csv> "<regex>"')
        sys.exit(1)

    output_file = sys.argv[1]
    regex = sys.argv[2]

    parsed_tree = parse_regex(regex)

    start_state, final_state = "q0"
    nfa = {}
    nfa, start, final = tree_to_nfa(parsed_tree, start_state, final_state, nfa)

    print(start)
    print(final)
    nfa[final]['output'] = "F"
    print(nfa)

    export_moore_automaton_to_csv(nfa, output_file, start)

if __name__ == "__main__":
    main()
