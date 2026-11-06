# Museu 3D em C — OpenGL

Museu modelado em C99 com OpenGL 2.1 e GLFW, seguindo a planta de referência. Toda a geometria é construída por vértices, paralelepípedos e malhas paramétricas no código, sem importar modelos. As texturas de piso, paredes, teto, pedra e terreno são procedurais; a textura terrestre da primeira exposição está em `assets`, com origem documentada em `assets/CREDITS.md`. O cálculo das luzes e da oclusão está em `src/lighting.c` e `include/lighting.h`.

## Organização e arquivos essenciais

```text
Projeto_CG/
├── src/                 # Implementação em C
│   ├── main.c           # Inicialização, opções e laço principal
│   ├── museum.c         # Estado inicial da sessão
│   ├── camera.c         # Câmera e deslocamento em primeira pessoa
│   ├── building.c       # Edifício, portão, janelas e colisões
│   ├── interaction.c    # Seleção pela mira, eventos e atualização das obras
│   ├── geometry.c       # Primitivas, transformações e aplicação dos materiais
│   ├── textures.c       # Geração das superfícies e carregamento da Terra
│   ├── textures.h       # Identificadores e interface interna das texturas
│   ├── artwork_render.c # Desenho das seis exposições
│   ├── scene.c          # Cena, luminárias, sombras, caches, mira e capturas
│   ├── render_internal.h # Interface interna dos módulos de desenho
│   ├── self_test.c      # Testes e sequência de animação do smoke test
│   ├── lighting.c       # Iluminação e sombras
│   ├── balance.c        # Lógica da balança
│   └── exhibits.c       # Lógica das outras quatro exposições
├── include/             # Cabeçalhos e parâmetros compartilhados
│   ├── museum.h         # Estado da sessão e constantes gerais
│   ├── building.h
│   ├── camera.h
│   ├── interaction.h
│   ├── render.h
│   ├── self_test.h
│   ├── platform_gl.h    # Inclusão de OpenGL/GLFW para Windows e Linux
│   ├── lighting.h
│   ├── earth.h
│   ├── balance.h
│   ├── exhibits.h
│   └── museum_layout.h
├── assets/              # Textura terrestre, original, créditos e licença
├── scripts/
│   └── build.ps1        # Compilação Windows com as ferramentas locais
├── docs/
│   └── oquefazer.txt    # Anotações e tarefas do projeto
├── CMakeLists.txt       # Compilação multiplataforma e testes
├── README.md
└── .gitignore
```

O `main.c` coordena a inicialização, as opções do terminal e o laço de execução. O estado compartilhado fica em `MuseumState`, definido em `include/museum.h` e inicializado em `src/museum.c`. Cada módulo mantém seus auxiliares privados com `static`; a planta e a lista de paredes pertencem a `src/building.c`, e os recursos de OpenGL ficam nos módulos de desenho. Os cabeçalhos de `include/` declaram as interfaces usadas entre módulos; `src/render_internal.h` reúne os detalhes internos de renderização.

Para **compilar pelo CMake**, mantenha `src/`, `include/`, `CMakeLists.txt` e `assets/` (a textura `earth.ppm`, `CREDITS.md` e `THREEJS-LICENSE.txt` são copiados para a distribuição). Também são necessários um compilador C99, OpenGL, GLU e GLFW, conforme as instruções abaixo. O JPEG original em `assets/earth-source.jpg` é uma referência para gerar a textura; o programa carrega apenas o PPM. `scripts/`, `docs/` e este README são arquivos de apoio.

Para **executar uma versão já compilada**, leve o executável e a pasta `assets/` produzida pelo build, mantendo-os lado a lado. No Windows, a compilação pelo script também gera `glfw3.dll`, que deve acompanhar o executável. No Linux, as bibliotecas de execução OpenGL, GLU e GLFW precisam estar disponíveis no sistema ou acompanhar a distribuição, conforme a compilação.

O `.gitignore` exclui `build/`, `build-linux/`, `build-wsl/`, outras pastas `build*` e `cmake-build*`, `.tools/`, binários e arquivos temporários. As pastas de build contêm executáveis e arquivos gerados, que podem ser recriados. `.tools/` contém as ferramentas usadas pelo script Windows e precisa estar disponível para usar esse script; o CMake também funciona com ferramentas instaladas no sistema. Código, textura, créditos, licença e documentação ficam no Git.

## Compilar e executar

Nesta máquina, o compilador Zig e a distribuição GLFW estão em `.tools`:

```powershell
.\scripts\build.ps1
.\build\museu3d.exe
```

Em outra máquina, instale CMake e um compilador C com bibliotecas OpenGL e GLU. No Windows, é possível usar Visual Studio Build Tools com a carga de trabalho **Desenvolvimento para desktop com C++**:

```powershell
cmake -S . -B build-cmake
cmake --build build-cmake --config Release
.\build-cmake\Release\museu3d.exe
```

O CMake baixa GLFW 3.4 na primeira configuração se ela não estiver instalada. Em geradores de configuração única, o executável fica diretamente em `build-cmake`. Use uma pasta de compilação nova ao trocar de compilador. Requer suporte a OpenGL 2.1 e GLU.

### Linux / WSL (Ubuntu, Debian ou Linux Mint)

Instale as dependências:

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config \
  libgl1-mesa-dev libglu1-mesa-dev xorg-dev \
  libwayland-dev libxkbcommon-dev wayland-protocols
```

No terminal Linux, dentro da pasta do projeto:

```bash
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release &&
cmake --build build-linux --parallel &&
./build-linux/museu3d
```

O projeto aceita CMake 3.20 ou superior, incluindo o 3.22 do Ubuntu 22.04. A pasta `build-linux` é separada da compilação Windows. No WSL, a janela precisa de suporte gráfico, como WSLg.

Para verificar a compilação: `ctest --test-dir build-linux --output-on-failure`. O teste de renderização também precisa de acesso à sessão gráfica.

### Modo leve para testar os objetos

Inicie com `--unlit` para usar cores diretas, sem cálculos de iluminação ou sombras:

```bash
./build-linux/museu3d --unlit --inside
```

Também pode combinar com qualquer posição inicial:

```bash
./build-linux/museu3d --unlit --earth
./build-linux/museu3d --unlit --balance
./build-linux/museu3d --unlit --sculpture
./build-linux/museu3d --unlit --pendulum
./build-linux/museu3d --unlit --gears
./build-linux/museu3d --unlit --colors
```

No Windows: `.\build\museu3d.exe --unlit --inside`. `--no-lighting` é um nome alternativo para a mesma opção. **F2** alterna entre o modo leve e a iluminação normal durante a execução, preservando o estado dos objetos e das fontes de luz. Feche uma execução antiga para abrir o executável atualizado.

O modo leve pula a iluminação por vértice, os raios de sombra e a coleta de oclusores. As caixas usam apenas suas seis faces, o terreno usa um único quadrilátero e as esferas e anéis usam menos divisões. O cenário estático deixa de ser reconstruído por causa do movimento dos objetos. Ao iniciar nesse modo, a janela é de 960 × 600 e não solicita antisserrilhamento por multisampling.

Navegação, colisões, janelas, portão e as seis obras continuam interativos. A textura terrestre é mantida. O laboratório RGB conserva os interruptores e o movimento da fonte verde, mas a projeção de cores e sombras aparece ao voltar à iluminação normal com **F2**.

## Edifício

- Salão principal retangular, corredor estreito e salão posterior mais largo, nas proporções aproximadas do desenho.
- Três janelas em cada lateral do salão principal, com posições e larguras assimétricas como na referência. Cada uma tem batente, duas folhas com painéis opacos e puxadores simples. Todas começam abertas para fora e abrem/fecham de forma independente por clique.
- Portão central de duas folhas metálicas gradeadas, com puxadores simples, abertura para dentro e animação por clique. Toda a geometria é feita com paralelepípedos.
- Quatro degraus retangulares largos para alcançar o piso elevado.
- Cobertura plana sólida e opaca, com espessura de 0,32, sobre os dois salões e o corredor. A face inferior forma um teto contínuo com seis luminárias gerais e quatro refletores de destaque.
- Interior escuro com iluminação quente uniforme no teto, nos rodapés e nas luzes diretas das obras; a luz solar entra pelos vãos abertos.
- Seis obras interativas distribuídas pelos salões, com percurso central livre entre a entrada e o salão posterior.

O eixo Y representa altura. A entrada fica em +Z e o salão posterior em −Z. As medidas são unidades de cena: salão principal de 16,4 × 20, corredor de 4,2 × 7,6, salão posterior de 17 × 12,4; paredes de 5 de altura e 0,28 de espessura. O piso fica a 0,72 do terreno, com degraus de 0,18. Os vãos de janela começam a 0,95 do piso e terminam a 3,65; a entrada tem 4,7 de largura e 3,5 de altura.

As quatro obras do salão principal ocupam os vértices de um retângulo de 8 × 10 no plano X/Z. Perto da entrada (Z=15), ficam a Terra à esquerda (X=−4) e a balança à direita (X=4). Perto do corredor (Z=5), ficam a escultura à esquerda e o pêndulo à direita, nas mesmas colunas. O caminho central continua livre. As posições são definidas em `include/museum_layout.h`; colisões, pontos de interação e luminárias acompanham as obras.

As dimensões da planta estão em `rooms`, os vãos em `left_windows`, `right_windows` e `entrance`, e as paredes são montadas em `build_museum`, em `src/building.c`. As mesmas peças definem a renderização e as colisões. A modelagem das obras está em `src/artwork_render.c`; as primitivas ficam em `src/geometry.c` e as texturas em `src/textures.c`.

## Controles

| Tecla | Ação |
| --- | --- |
| W/A/S/D | Caminhar |
| Mouse | Olhar ao redor |
| Setas ←/→ | Girar a visão para a esquerda/direita |
| Setas ↑/↓ | Olhar para cima/baixo |
| Clique esquerdo | Abrir/fechar o portão ou a janela em que estiver mirando, a até 6 unidades |
| Segurar clique esquerdo sobre a Terra | Girar o globo enquanto o botão estiver pressionado |
| Segurar clique esquerdo sobre uma bola da balança | Aumentar/diminuir apenas essa bola em ciclo |
| Segurar clique esquerdo nas novas obras | Desmontar a escultura, puxar o pêndulo, girar engrenagens ou mover a fonte verde |
| R | Restaurar a balança e as quatro novas obras |
| Shift esquerdo | Correr |
| L | Ligar/desligar a luz solar direcional |
| 1 | Ligar/desligar as seis luzes do teto |
| 2 | Ligar/desligar todas as faixas de rodapé do museu |
| 3 | Ligar/desligar o iluminador da Terra no chão |
| 4 / 5 / 6 | Alternar, respectivamente, as luzes vermelha, verde e azul do laboratório |
| 7 | Ligar/desligar os quatro refletores de destaque das obras |
| F2 | Alternar modo leve sem iluminação/sombras |

A câmera acompanha a altura dos degraus e permanece a 1,7 acima do piso. Olhar para cima ou para baixo não altera o deslocamento horizontal. Há colisões nas paredes, nos peitoris e nas laterais da plataforma. A aplicação permanece em primeira pessoa, com o cursor capturado e a cobertura sempre visível.

Mantenha as setinhas pressionadas para girar a visão continuamente, mesmo sem usar o mouse. A rotação tem velocidade de 90° por segundo e a inclinação fica limitada para evitar virar a câmera de cabeça para baixo.

Aproxime-se da entrada e mire no portão: a mira fica dourada quando o clique está disponível. Um clique esquerdo abre as duas folhas; outro fecha ou inverte a animação. Os cliques usam sempre a mira central. O acionamento funciona por dentro e por fora.

As seis janelas usam o mesmo controle: aproxime-se e mire em uma folha ou no vão para clicar. Cada clique controla somente a janela selecionada; o portão e as demais janelas mantêm seus estados. O acionamento funciona por dentro e por fora, pela mira central. Paredes bloqueiam a seleção de objetos escondidos.

As folhas têm colisão mesmo durante a rotação. Se o personagem ocupar o arco de uma folha, o movimento pausa e continua quando ele se afasta. O portão inicia fechado. Para inspeção, use `--gate-open` ao iniciar, por exemplo `./build-linux/museu3d --inside --gate-open` no Linux.

As folhas das janelas também têm colisão e pausam antes de atingir o personagem. As janelas começam a 90°; um clique fecha até 0° e outro reabre, podendo inverter o movimento durante a animação. Para inspecioná-las fechadas desde o início, use `./build-linux/museu3d --inside --windows-closed`.

A câmera possui apenas o modo de caminhada em primeira pessoa. Não há comandos para liberar o cursor, mudar a perspectiva ou ocultar o teto, incluindo opções de inicialização.

## Materiais e iluminação

| Superfície | Textura |
| --- | --- |
| Piso dos dois salões e do corredor | Madeira clara com veios e tábuas desencontradas, de 0,32 × 2,56 unidades |
| Paredes internas e externas | Reboco pintado em tom claro, com granulação fina |
| Teto | Placas acústicas claras de 0,60 × 0,60 unidades, com juntas e pequenos pontos |
| Degraus e caminho da entrada | Pedra clara com granulação e juntas de 1 unidade |
| Terreno | Grama com variação de cor |

As cinco texturas de superfície são geradas em `src/textures.c`, com 256 × 256 pixels cada, apenas na inicialização. Não precisam de imagens adicionais em `assets/`, downloads ou novas bibliotecas. Os detalhes de juntas e perfurações são visuais: as superfícies mantêm a mesma geometria sólida e as mesmas colisões. A textura terrestre continua sendo carregada de `assets/earth.ppm`.

As coordenadas UV usam as posições na cena para manter a escala e o alinhamento entre os blocos das paredes e os pisos conectados. Cada material tem seu tamanho de repetição; o mapeamento considera a orientação de cada face. A filtragem usa mipmaps para reduzir a cintilação à distância. As cores das texturas são combinadas com a iluminação quente existente e também aparecem no modo leve, sem aumentar as subdivisões nem executar cálculos de luz ou sombra nesse modo. Materiais das obras, emissores de luz e superfícies brancas do laboratório RGB preservam suas cores.

As janelas usam painéis sólidos e opacos com espessura, desenhados junto com suas molduras e acompanhando a rotação de cada folha. Quando fechadas, não é possível enxergar através delas. A cobertura também é sólida e opaca, sem vidro ou grade metálica. A cena usa materiais com alfa 1 e teste/gravação de profundidade normais, sem passagem de transparência.

A iluminação ambiente interna é baixa e quente. As seis luminárias gerais do teto e todas as faixas de rodapé usam exatamente a mesma cor, definida por `WARM_COLOR` em `src/lighting.c`. Os difusores preservam essa tonalidade ao serem desenhados, evitando que a emissão sature para branco. Há quatro fontes gerais no teto do salão principal e duas no salão posterior, com cones suaves e atenuação por distância.

O rodapé luminoso foi estendido em 13 trechos ao longo do contorno interno dos dois salões e do corredor, a 0,18 acima do piso. As faixas acompanham paredes horizontais e verticais, encontram-se nos cantos e são interrompidas na porta. Os acessos ao corredor e a circulação central permanecem livres. A geometria de cada faixa e sua luz usam os mesmos pontos de início e fim.

Os quatro refletores de destaque — escultura, pêndulo, balança e engrenagens — e a luz ascendente da Terra também usam a mesma cor quente do teto e dos rodapés: `EXHIBIT_COLOR` referencia `WARM_COLOR`. Os refletores ficam ligeiramente à frente das obras e apontam para elas. O laboratório RGB conserva as cores do experimento.

**1** alterna as seis luzes gerais do teto; **2**, todas as faixas de rodapé; **3**, o iluminador da Terra; **7**, os quatro refletores das demais obras. **L** alterna a luz solar. Para comparar o ambiente quente sem os refletores de destaque, use `--inside --no-exhibit-lights`. O modo leve e **F2** continuam disponíveis e não calculam nenhuma dessas fontes.

A luz direcional externa vem de cima e do lado esquerdo do edifício. Ela entra apenas onde um raio consegue atravessar um vão: abrir uma janela desse lado cria uma área de sol no interior; fechar a folha remove essa entrada de luz. As janelas do lado oposto não recebem sol direto nessa direção. Paredes, teto, portão e folhas das janelas ocluem a iluminação. As pequenas folgas dos batentes são vedadas no cálculo de sombra quando uma janela está totalmente fechada.

Para manter a compatibilidade com OpenGL 2.1, as faces são subdivididas e a iluminação difusa é calculada em C nos vértices, incluindo testes de visibilidade contra as caixas e esferas da própria geometria. O OpenGL interpola as cores entre os vértices. A iluminação estática fica em uma lista de desenho e é recalculada quando as folhas ou os grupos de luz mudam. Durante as animações das novas obras, as sombras do cenário são atualizadas em intervalos de aproximadamente 0,12 segundo e os objetos são redesenhados na posição atual. As sombras têm a resolução da malha, sem reflexão indireta ou penumbra física.

Para adicionar ou ajustar fontes, edite a tabela `museum_lights` em `src/lighting.c`: cada entrada especifica tipo, posição, fim do segmento ou direção, cor, intensidade, alcance e normal de montagem. Os tipos são fontes gerais voltadas para baixo (`LIGHT_SPOT`), faixas lineares (`LIGHT_STRIP`), iluminação ascendente da Terra (`LIGHT_UP_DIRECTIONAL`) e refletores orientáveis de obras (`LIGHT_EXHIBIT_SPOT`). Nos dois últimos, `end` define a direção dos raios. Nas faixas, `mounting_normal` aponta para o interior e posiciona a carcaça atrás do emissor.

## Primeira exposição: Terra flutuante

O globo fica no salão principal, em `(-4; 3,12; 15)`, com raio 1,25 e sem suporte entre a esfera e o chão. Há espaço para circular ao redor e a esfera tem colisão. A malha é construída com 48 faixas de latitude e 96 divisões de longitude. As coordenadas UV aplicam uma imagem equiretangular da Terra, com oceanos, continentes e nuvens.

Na primeira pessoa, aproxime-se e mire na esfera: a mira fica dourada. **Segure o botão esquerdo para girar a Terra a 45° por segundo; solte para parar**, mantendo a orientação atingida. Funciona pela mira central. Perder o foco da janela ou afastar-se além do alcance cancela a rotação. Durante o pressionamento, o movimento do mouse não gira a câmera.

O iluminador fica no chão, centralizado no mesmo X e Z da Terra. Ele emite raios paralelos verticalmente para cima na mesma cor quente das demais luminárias, com alcance e área limitados à exposição. A iluminação é mais forte na parte inferior da Terra e a esfera projeta sombra. **3** alterna essa luz sem desligar as outras fontes. Um preenchimento discreto no material mantém os continentes legíveis na face escura.

Para iniciar diante da peça no Linux:

```bash
./build-linux/museu3d --earth
```

Para inspeção e capturas, `--earth-turned` inicia com uma rotação de 100° e `--no-earth-light` desliga o iluminador. Os builds copiam `assets/earth.ppm` para a pasta `assets` junto ao executável. Ao transportar o executável, leve essa pasta também. A aplicação não baixa imagens durante a execução e não precisa de uma biblioteca de imagens: lê o PPM RGB diretamente.

No Linux, algumas opções ajudam a comparar os efeitos:

```bash
./build-linux/museu3d --inside
./build-linux/museu3d --inside --windows-closed
./build-linux/museu3d --corridor
./build-linux/museu3d --inside --no-sun
./build-linux/museu3d --inside --no-ceiling --no-strips
```

`--inside` inicia no salão principal e `--corridor` inicia diante do corredor. Também podem ser combinados com `--screenshot caminho.ppm`. A cobertura permanece visível e bloqueia a luz externa.

## Segunda exposição: balança

A balança fica no salão principal, do lado oposto à Terra, sob uma das luminárias do teto, em `(4; 0,72; 15)`. A modelagem usa uma base, haste, braço, tirantes e dois pratos simples, com uma bola vermelha à esquerda e uma azul à direita. Os pratos ficam horizontais enquanto acompanham a inclinação do braço.

Na primeira pessoa, mire em uma bola e **segure o botão esquerdo**. Somente essa bola muda de tamanho: cresce até o raio máximo, diminui até o mínimo e repete o ciclo enquanto o botão continuar pressionado. Soltar mantém o tamanho alcançado. Cada bola conserva seu tamanho e sentido de variação de forma independente. Perder o foco ou afastar-se cancela o pressionamento.

As duas começam com raio **0,45**, variando entre **0,30 e 0,85**, a uma velocidade de 0,24 unidade de raio por segundo. Esses parâmetros estão em `include/balance.h`, especialmente `BALANCE_MAX_RADIUS`, que define o tamanho máximo solicitado. Nenhum modelo é importado.

O braço pende suavemente para a bola maior. O peso relativo é calculado pelo volume (`raio³`), considerando o mesmo material e braços iguais, com inclinação limitada a 22°. Quando os tamanhos são iguais, a balança retorna à horizontal. **R** restaura imediatamente as duas bolas e o equilíbrio inicial. As bolas, pratos e braço também participam do cálculo de sombras.

Para iniciar diante da balança:

```bash
./build-linux/museu3d --balance
```

Para inspecionar os dois lados inclinados, acrescente `--balance-left-heavy` ou `--balance-right-heavy`. A lógica independente da renderização fica em `src/balance.c`.

## Outras quatro exposições

O museu agora tem seis obras. As novas peças são inteiramente procedurais, sem importar modelos. A lógica de interação, posições e animação está em `src/exhibits.c` e `include/exhibits.h`.

| Obra | Localização | Interação e conceito |
| --- | --- | --- |
| Escultura desmontável | Salão principal, à esquerda perto do corredor, em `(-4; 2,35; 5)` | Segure qualquer cubo para separar os 27 blocos em X, Y e Z; solte para recompor a escultura. O tamanho de cada peça permanece constante: a transformação é translação. |
| Pêndulo | Salão principal, à direita perto do corredor, com suporte em X=4 e Z=5 | Segure a esfera para puxá-la. Mova o mouse lateralmente ou use ←/→ para escolher o lado. Sem arrastar, segurar também a eleva gradualmente. Ao soltar, ela oscila e perde amplitude até parar. A haste mantém comprimento fixo. |
| Engrenagens | Salão posterior, lado esquerdo | Segure qualquer roda para mover o conjunto. As três rodas têm 16, 10 e 12 dentes, com velocidades inversamente proporcionais e sentidos alternados. Soltar interrompe a rotação. |
| Laboratório RGB | Salão posterior, lado direito | Clique nos três botões coloridos da bancada para alternar R/G/B. Segure a luminária verde para movê-la lateralmente e solte para manter a posição. As fontes reais iluminam a escultura branca e produzem sombras coloridas no painel. |

As ações funcionam na primeira pessoa, a até seis unidades, usando a mira central. Soltar o botão, perder foco ou afastar-se encerra o acionamento; o pêndulo continua sua oscilação e a escultura se recompõe após a liberação. **R** restaura a balança e as quatro novas exposições. Os canais RGB também podem ser alternados por **4**, **5** e **6**.

Para iniciar diante de cada obra no Linux:

```bash
./build-linux/museu3d --sculpture
./build-linux/museu3d --pendulum
./build-linux/museu3d --gears
./build-linux/museu3d --colors
```

Para capturas de estados específicos, combine com `--sculpture-open`, `--pendulum-pulled`, `--gears-turned`, `--colors-red-only` ou `--colors-moved`, e `--screenshot caminho.ppm`. As capturas mantêm as quatro novas obras no estado escolhido, sem avançar sua animação. Na execução normal, uma escultura inicialmente aberta se recompõe e um pêndulo inicialmente puxado começa a oscilar.

O laboratório usa três fontes posicionais direcionadas para o painel branco. Cada contribuição R/G/B é calculada separadamente com atenuação, cone e oclusão. Alterar a posição da fonte verde muda as regiões de sobreposição e as sombras. A luminária branca do teto nessa região foi posicionada junto ao acesso, para preservar a leitura da mistura de cores no painel.

## Transformações geométricas das obras

O sistema de coordenadas usa **X** para esquerda/direita, **Y** para altura e **Z** para a direção entre a entrada e os fundos do museu. As obras combinam translação (mudança de posição), rotação (mudança de orientação) e escala (mudança de tamanho). Algumas transformações são aplicadas pelas matrizes do OpenGL; outras são calculadas diretamente nas posições dos vértices ou dos centros das peças.

| Obra | Transformações usadas | Funções principais |
| --- | --- | --- |
| Terra | Rotação visual em torno de Y, por deslocamento da textura | `update_earth`, `draw_earth` |
| Balança | Escala uniforme das bolas, rotação do braço em Z e translação dos pratos/bolas | `balance_update`, `balance_target_angle`, `balance_endpoint`, `balance_ball_center`, `draw_balance` |
| Escultura desmontável | Translação dos 27 cubos em X, Y e Z | `exhibits_update`, `sculpture_piece`, `draw_exhibit_dynamic` |
| Pêndulo | Rotação em Z ao redor do ponto de suspensão e translação da esfera em arco | `exhibits_update`, `pendulum_bob`, `draw_exhibit_dynamic` |
| Engrenagens | Rotações em Z com velocidades proporcionais ao número de dentes | `gear_angle`, `exhibits_update`, `draw_exhibit_dynamic` |
| Laboratório RGB | Translação da luminária verde em X e atualização da posição da luz | `exhibits_update`, `rgb_lamp`, `lighting_set_rgb` |

### Composição das transformações e pivôs

Em `src/geometry.c`, `box` desenha um cubo unitário centrado na origem, com coordenadas entre −0,5 e 0,5. `glScaled(size.x, size.y, size.z)` define suas dimensões e `glTranslated(pos.x, pos.y, pos.z)` o posiciona. Essa escala pode ser diferente em cada eixo para formar uma haste, um prato ou um pedestal.

Para desenhar peças que giram, `object_begin` aplica uma translação até o pivô e uma rotação em Y. `object_begin_roll` usa a mesma preparação e acrescenta uma rotação em Z com `glRotated(angle, 0, 0, 1)`. Cada grupo é delimitado por `glPushMatrix` e `glPopMatrix`, de modo que as transformações de uma peça não alterem o desenho das seguintes.

Considerando vetores-coluna, a parte de modelagem de uma peça desenhada por `box` dentro de `object_begin_roll` é:

```text
p_mundo = T(pivô) · Rz(ângulo) · T(centro_local) · S(dimensões) · p_unitário
```

Embora as chamadas sejam feitas na ordem translação do pivô, rotação, translação local e escala, a composição atua sobre o vértice da direita para a esquerda. Assim, primeiro a peça ganha tamanho, depois é posicionada em relação ao pivô, gira e finalmente é colocada no museu. Os ângulos passados para `glRotated` são em graus; `sin` e `cos` recebem radianos.

### Terra: rotação pela matriz de textura

Em `src/interaction.c`, `update_earth` aumenta `museum.earth_angle` somente enquanto o usuário segura a esfera:

```text
ângulo = (ângulo + 45 · dt) módulo 360
```

Em `draw_earth`, em `src/artwork_render.c`, esse ângulo desloca a coordenada horizontal U da textura equiretangular. O código seleciona `GL_TEXTURE`, salva a matriz e executa `glTranslated(-ângulo / 360, 0, 0)`. Depois do desenho, restaura a matriz de textura e volta a `GL_MODELVIEW`. Uma volta completa corresponde a deslocar U em uma unidade; a repetição horizontal da textura fecha o ciclo.

O resultado visual é a Terra girando em torno do eixo Y. A malha esférica e seu centro permanecem fixos, enquanto os continentes mudam de posição na superfície. Isso permite manter os cálculos de iluminação e a sombra associados à esfera no espaço do museu. A altura de flutuação é definida por `EARTH_Y`; não há uma animação de subida e descida.

### Balança: escala das bolas e rotação do braço

Em `src/balance.c`, cada bola possui um raio independente. Enquanto ela está pressionada, `balance_update` varia esse raio a 0,24 unidade por segundo entre 0,30 e 0,85, usando um ciclo triangular: cresce, alcança o máximo, diminui e volta a crescer. Ao soltar, o raio fica no valor atingido.

A escala uniforme é aplicada diretamente na geração dos vértices de `draw_sphere`, em `src/geometry.c`:

```text
vértice = centro_da_bola + raio · normal_da_esfera_unitária
```

Multiplicar as três componentes pelo mesmo raio aumenta ou diminui a esfera sem deformá-la. O centro da bola também sobe ou desce conforme o raio, mantendo sua parte inferior apoiada no prato.

Para inclinar o braço, `balance_target_angle` considera o peso proporcional ao volume. Como as bolas têm o mesmo material, o fator comum do volume da esfera se cancela na comparação:

```text
ângulo_alvo = 22° · (raio_esquerdo³ − raio_direito³) / (0,85³ − 0,30³)
```

O ângulo atual se aproxima suavemente desse alvo. `draw_balance` usa `object_begin_roll` para girar o braço em Z ao redor de `(BALANCE_X, BALANCE_PIVOT_Y, BALANCE_Z)`. Para cada lado, `balance_endpoint` calcula a extremidade do braço, com `s = −1` à esquerda e `s = +1` à direita:

```text
x = BALANCE_X + s · BALANCE_ARM · cos(ângulo)
y = BALANCE_PIVOT_Y + s · BALANCE_ARM · sin(ângulo)
z = BALANCE_Z
```

Essas posições determinam a translação dos pratos e das bolas. Os pratos permanecem horizontais e os tirantes verticais enquanto acompanham as extremidades. Quando os raios são iguais, o ângulo-alvo é zero e a balança retorna ao equilíbrio.

### Escultura desmontável: translação de cada cubo

Em `src/exhibits.c`, `sculpture_piece` distribui os 27 cubos em uma grade 3 × 3 × 3. Cada peça recebe índices locais `ix`, `iy` e `iz` entre −1 e 1. O parâmetro `a = sculpture_open` varia de 0, montada, até 1, desmontada:

```text
distância_entre_centros = 0,43 · (1 + 2 · a)
x = SCULPTURE_X + ix · distância_entre_centros
y = SCULPTURE_Y + iy · distância_entre_centros + 0,95 · a
z = SCULPTURE_Z + iz · distância_entre_centros
```

Ao segurar um cubo, `exhibits_update` aproxima `a` de 1; ao soltar, aproxima de 0. O espaçamento aumenta de 0,43 para 1,29 e o conjunto sobe até 0,95 unidade. Cada cubo continua com dimensões de 0,40 × 0,40 × 0,40, definidas em `draw_exhibit_dynamic`. A abertura resulta da translação dos centros das peças; suas dimensões permanecem constantes durante a animação.

### Pêndulo: rotação ao redor da suspensão

A haste gira no plano XY, em torno do eixo Z, com pivô em `(PENDULUM_X, PENDULUM_Y, PENDULUM_Z)`. `draw_exhibit_dynamic` posiciona a origem nesse pivô e chama `object_begin_roll` com o ângulo convertido de radianos para graus.

A esfera acompanha a ponta da haste por uma posição calculada em `pendulum_bob`:

```text
x = PENDULUM_X + comprimento · sin(ângulo)
y = PENDULUM_Y − comprimento · cos(ângulo)
z = PENDULUM_Z
```

O comprimento é fixo em 2,6 unidades. Assim, o centro da esfera percorre um arco de circunferência. Segurar e arrastar com o mouse, ou usar as setas laterais, altera o ângulo de controle dentro de ±0,95 radiano. Segurar sem arrastar também aumenta o ângulo gradualmente.

Ao soltar, `exhibits_update` integra a aceleração angular `−(9,81 / comprimento) · sin(ângulo) − 0,38 · velocidade_angular`. O primeiro termo representa a ação da gravidade e o segundo o amortecimento. A atualização usa subpassos de até 0,008 segundo, produzindo a oscilação com perda gradual de amplitude.

### Engrenagens: rotações relacionadas pelo número de dentes

Os centros das três rodas ficam fixos. Cada roda gira em Z ao redor do próprio centro, com a translação até esse centro seguida da rotação. Os dentes são distribuídos ao redor da roda por ângulos adicionais `dente · 360° / número_de_dentes`, e os quatro raios internos recebem deslocamentos angulares de 90°.

Em `gear_angle`, em `src/exhibits.c`, uma fase compartilhada `φ` relaciona os ângulos:

```text
ângulo_i = sinal_i · φ · 16 / dentes_i + ajuste_i
dentes = [16, 10, 12]
sinais = [+1, −1, +1]
ajustes = [0°, 18°, 15°]
```

O resultado é reduzido módulo 360°. O fator inversamente proporcional ao número de dentes faz a roda menor girar mais rápido, e os sinais fazem rodas vizinhas girarem em sentidos opostos. Os ajustes iniciais alinham os dentes. Ao segurar uma roda, a fase é atualizada para que a roda selecionada gire a 60° por segundo; ao soltar, a fase para de mudar.

### Laboratório RGB: translação da luminária e da fonte de luz

Em `exhibits_update`, a coordenada `green_x` da luminária verde percorre o intervalo de 2,6 a 6,4, a 1,2 unidade por segundo, invertendo o sentido nos limites enquanto o usuário segura a fonte. Sua altura permanece em `RGB_GREEN_Y = 4,05` e Z permanece em −12,5. `rgb_lamp` retorna essa posição para desenhar a luminária e seu suporte.

Em `src/scene.c`, `lighting_set_rgb` recebe a mesma coordenada X para atualizar a fonte usada no cálculo de iluminação. Dessa forma, a geometria da luminária e a luz se deslocam juntas, alterando a sobreposição das cores e as sombras no painel. Ao soltar, a posição é mantida. Os botões RGB alteram o estado de cada canal de iluminação.

### Tempo de animação e suavização

As atualizações recebem `dt`, o tempo decorrido em segundos. Velocidades são multiplicadas por esse tempo para manter o ritmo das animações mesmo com taxas de quadros diferentes. Na escultura e na inclinação da balança, a aproximação ao alvo usa `valor += (alvo − valor) · (1 − exp(−k · dt))`, com `k = 4` e `k = 6`, respectivamente, evitando mudanças bruscas. A lógica das animações fica separada do desenho, principalmente em `src/exhibits.c`, `src/balance.c` e `src/interaction.c`.

## Verificações e capturas

Os testes de integração e a sequência do teste de renderização estão em `src/self_test.c`. As verificações independentes de iluminação, balança e animações continuam em seus respectivos módulos.

```powershell
.\build\museu3d.exe --self-test
.\build\museu3d.exe --smoke-test
.\build\museu3d.exe --inside --screenshot build\salao.ppm
.\build\museu3d.exe --corridor --screenshot build\corredor.ppm
```

O primeiro teste verifica os seis vãos, entrada, colisões, subida e descida da escada, travessia entre os salões, altura da câmera e diagonal normalizada. Também verifica cliques válidos e inválidos, interação pelo interior, rotação das folhas, bloqueio da passagem com o portão fechado e pausa do fechamento ao encontrar o personagem. Para cada janela, verifica estado inicial aberto, cliques independentes por dentro e por fora, inversão da animação, alcance, colisão das folhas e oclusão por paredes. O segundo desenha seis quadros em janela oculta, alternando posições em primeira pessoa, luz solar, portão e janelas em diferentes ângulos, sempre com a cobertura visível, e verifica erros OpenGL. As capturas são salvas em PPM, sem abrir janela.

Os testes também verificam alcance e oclusão das luzes, ambiente escuro, bloqueio por caixas giradas, sol passando por uma janela aberta e sendo bloqueado ao fechá-la, independência da janela vizinha, oclusão pelo teto e iluminação quente do piso pelas faixas. Os rodapés são verificados nos dois salões e no corredor, acompanhando paredes e sem atravessar a entrada. Há uma verificação separada para a contribuição dos refletores de obras. A renderização alterna os grupos de luz interna.

Para a Terra, os testes verificam seleção por raio, rotação somente enquanto o botão está pressionado, parada ao soltar, cancelamento ao perder foco, alcance, colisão, bloqueio por parede, iluminação do hemisfério inferior e sombra da esfera. Também verificam que as teclas removidas preservam a interação, a mira e a cobertura. O teste de renderização carrega a textura e desenha o globo em diferentes orientações e estados de iluminação.

Para a balança, os testes verificam os limites e inversões do ciclo de cada bola, independência entre os lados, pausa ao soltar, seleção das esferas, cancelamento, colisões e retorno ao equilíbrio. Também verificam que o lado maior desce, a oclusão de caixas inclinadas no eixo do braço e as sombras de várias esferas. A renderização inclui os estados equilibrado, esquerda maior e direita maior.

As quatro obras adicionais têm testes para translação e retorno dos cubos, comprimento e amortecimento do pêndulo, proporções e sentidos das engrenagens, controles e limites da fonte móvel, seleção por raio de todas as peças e botões, acionamento exclusivo, liberação, alcance, foco, colisões e restauração por **R**. A iluminação é verificada com canais RGB separados e fonte deslocada. A renderização alterna as poses e as combinações de cores, mantendo a rota central livre.

O teste `rendering_unlit` executa as posições de caminhada e animações em modo leve e verifica que ocorreram **zero amostras de iluminação e zero raios de sombra**. O teste de renderização normal também alterna **F2** para verificar a reconstrução dos caches ao sair e voltar ao modo iluminado. Para executar só o teste leve: `./build-linux/museu3d --smoke-test --unlit`.

Com CMake: `ctest --test-dir build-cmake -C Release --output-on-failure`.
