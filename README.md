# 📳 Chat cliente servidor em C 

O cliente deve ser capaz de se conectar ao servidor para enviar e receber mensagens
Trabalho desenvolvido para a disciplina de Tecnologia de Comunicação de Dados
---

## 🛠️ Como Rodar Localmente

1. **Clone o projeto**
   ```bash
   git clone https://github.com/MatheusZuccon/Chat_Client_Server
   cd Chat_Client_Server

2. **Rode no terminal linux ou WSL integrado, threads não existem no windows**
   ```bash
   gcc -Wall -Wextra -g -pthread -o server server.c
   em outro terminal rode o client.c
   gcc -Wall -Wextra -g -pthread -o client client.c


