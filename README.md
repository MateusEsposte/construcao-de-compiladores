
# Analisadores Léxico e Sintático COBOL

Projeto desenvolvido para a disciplina de Construção de Compiladores, com o
objetivo de implementar analisadores léxico e sintático em C para código-fonte
COBOL.

O programa identifica e classifica elementos léxicos do código, como palavras-chave, identificadores, literais numéricos e alfanuméricos, símbolos de pontuação e tokens inválidos.

## Tecnologias

- Linguagem C
- GCC (compilador C)

## Autores

- **Mateus da Cruz Esposte** — 13862650
- **Nazivon Santos do Nascimento** — 10266548

## Descrição

O scanner lê arquivos no formato fixo COBOL: ignora as colunas 1–6, usa a
coluna 7 como indicador e analisa o texto das colunas 8–72. Linhas de
comentário com `*` ou `/` e linhas de depuração com `D` são ignoradas. O
indicador `-` é reconhecido na continuação de literais.

As categorias e regras básicas seguem a especificação do COBOL fornecida para o projeto: palavras reservadas, identificadores de até 30 caracteres,
literais numéricos com sinal e decimal, literais alfanuméricos e pontuação.
Foram acrescentados tokens para palavras reservadas encontradas nos exemplos,
operadores aritméticos e relacionais e o separador `:`.

`TOK_KEYWORD` identifica palavras reservadas adicionais que não têm um tipo
individual; o texto original permanece em `Token.lexeme` para que etapas
posteriores possam diferenciá-las. O scanner também aceita `_` dentro de
identificadores para cobrir os nomes de programa `PROG005_strings`,
`PROG006_files` e `PROG007_loja` do corpus. Essa é uma extensão de
compatibilidade: a especificação do projeto descreve identificadores com
letras, dígitos e hífen. Sublinhado no início ou no fim continua inválido.

O parser implementa análise sintática descendente para a estrutura das
divisões e seções e para as declarações e instruções usadas pelo corpus de
exemplos: dados, expressões, operações aritméticas, entrada/saída, condições,
laços, busca e manipulação de arquivos e strings. O programa não constrói uma
AST nem faz análise semântica.

## Compilar e executar

No PowerShell, dentro desta pasta:

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror main.c parser.c lexer.c token.c -o compilador.exe
.\compilador.exe .\teste.cob
.\compilador.exe --tokens .\teste.cob
```

Se o caminho não for informado, o programa tenta abrir `teste.cob` no
diretório atual. O programa informa erros sintáticos e léxicos com a posição
no arquivo e retorna código `1` quando encontra erros. A opção `--tokens`
mantém a saída detalhada do analisador léxico.

## Testes

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror test_lexer.c lexer.c token.c -o test_lexer.exe
.\test_lexer.exe
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror test_parser.c parser.c lexer.c token.c -o test_parser.exe
.\test_parser.exe
```

A bateria cobre palavras-chave e identificadores, operadores, números com
sinal, literais com aspas duplicadas, pontuação, comentários, limites de
identificadores, números longos e identificadores COBOL que começam por
dígitos. Os testes do parser verificam programas válidos e rejeitam entradas
com erros sintáticos.

## Demonstração de erros léxicos

`erros_lexicos.cob` é um arquivo separado dos exemplos válidos e contém dez
casos para demonstrar `TOK_ERROR`: caracteres não reconhecidos, sublinhado
isolado, identificadores com hífen ou sublinhado no final, identificador acima
do limite de 30 caracteres e literal sem fechamento.

Na raiz do repositório, execute:

```powershell
.\programa\compilador.exe --tokens .\programa\erros_lexicos.cob
```

O scanner imprime os tokens e suas posições e retorna código de saída `1`,
como esperado para esse arquivo inválido.

## Corpus de exemplos

Os sete arquivos listados no
[repositório ebasso/cobol-exemplos](https://github.com/ebasso/cobol-exemplos)
foram executados com o lexer.
Todos terminaram com código de saída `0` e nenhum `TOK_ERROR`:

| Arquivo | Erros léxicos | Análise sintática |
|---|---:|---|
| `prog001_divisoes.cob` | 0 | OK |
| `prog002_operacoes.cob` | 0 | OK |
| `prog003_calculadora.cob` | 0 | OK |
| `prog004_folha_pgto.cob` | 0 | OK |
| `prog005_strings.cob` | 0 | OK |
| `prog006_files.cob` | 0 | OK |
| `prog007_loja.cob` | 0 | OK |

Os sete fontes COBOL originais também foram usados para validar a análise
sintática. O parser reconhece os comandos e construções presentes nesses
exemplos, mas não cobre toda a linguagem COBOL nem valida a semântica dos
programas.
