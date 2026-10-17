# Analisador léxico COBOL

O scanner lê arquivos no formato fixo COBOL: ignora as colunas 1–6, usa a
coluna 7 como indicador e analisa o texto das colunas 8–72. Linhas de
comentário com `*` ou `/` e linhas de depuração com `D` são ignoradas. O
indicador `-` é reconhecido na continuação de literais.

As categorias e regras básicas seguem a `EspecificacaoCOBOL.pdf` na raiz do
repositório: palavras reservadas, identificadores de até 30 caracteres,
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

## Compilar e executar

No PowerShell, dentro desta pasta:

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror main.c lexer.c token.c -o lexer.exe
.\lexer.exe .\teste.cob
```

Se o caminho não for informado, o programa tenta abrir `teste.cob` no
diretório atual. O código de saída é `1` quando o scanner encontra algum
`TOK_ERROR`.

## Testes

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror test_lexer.c lexer.c token.c -o test_lexer.exe
.\test_lexer.exe
```

A bateria cobre palavras-chave e identificadores, operadores, números com
sinal, literais com aspas duplicadas, pontuação, comentários, limites de
identificadores e números longos.

## Corpus de exemplos

Os sete arquivos listados no
[repositório ebasso/cobol-exemplos](https://github.com/ebasso/cobol-exemplos)
foram executados com o lexer (revisão `dbe15b74769c994ceda82b48b2640fb208907d3c`).
Todos terminaram com código de saída `0` e nenhum `TOK_ERROR`:

| Arquivo | Erros léxicos |
|---|---:|
| `prog001_divisoes.cob` | 0 |
| `prog002_operacoes.cob` | 0 |
| `prog003_calculadora.cob` | 0 |
| `prog004_folha_pgto.cob` | 0 |
| `prog005_strings.cob` | 0 |
| `prog006_files.cob` | 0 |
| `prog007_loja.cob` | 0 |

Esse resultado avalia somente a análise léxica: não valida a sintaxe nem a
semântica dos programas COBOL.
