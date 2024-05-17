#include <iostream>
#include <fstream>
#include <SDL2/SDL.h>
#include <windows.h>

//Chip 8 System Struct
struct Chip8System {
    //Memory (The Chip-8 has 4 Kb, or 4096 bytes)
    uint8_t Chip8Memory[4096];

    //Program Counter and Index Register Initilization
    uint16_t I = 0; //The PC and Index Register can actually only address 12 bits
    uint16_t PC = 0x200; //First location of memory allocated for Chip8 should be loaded into x200.
    uint16_t SP = 15; //Defaults to top of the stack

    //Register Initilization (V0-VF)
    uint8_t V[16]; //Its better to use this an array rather than a bunch of variables since it is easier to manage.

    //Timers Initilization
    uint8_t DelayTimer, SoundTimer = 0;

    //Stack
    uint16_t Stack[16]; //Limited Stack Space

    //Display
    uint8_t Chip8Display[64][32]; //The display only needs each pixel to be on or off, so it is better to use the smallest variable possible.
    uint8_t DisplayUpdate = 0; //The flag only needs to be on or off, so it is better to use the smallest variable possible.

    //Keypad State; Each Key is either on or off.
    uint8_t Chip8KeyPad[16];
    uint8_t keymap[16] = { 
    //Keymap for the Chip-8 system
    SDLK_1, SDLK_2, SDLK_3, SDLK_4, //Key Presses 1, 2, 3, C
    SDLK_q, SDLK_w, SDLK_e, SDLK_r, //Key Presses 4, 5, 6, D
    SDLK_a, SDLK_s, SDLK_d, SDLK_f, //Key Presses 7, 8, 9, E
    SDLK_z, SDLK_x, SDLK_c, SDLK_v  //Key Presses A, 0, B, F
}; 

    //Font, will be loaded into memory locations 0x050 to 0x09F
    uint8_t FONT[80] {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    //Display Variables
    int WIDTH = 64, HEIGHT = 32, scalefactor;
};

//System Function Declarations
void Chip8DisplayOut(Chip8System *Chip8, SDL_Renderer *renderer);
void Chip8Init(Chip8System *Chip8);
void Chip8CPU(Chip8System *Chip8, uint16_t opcode);
void Chip8UpdateTimers(Chip8System *Chip8);
void Chip8Keyboard(Chip8System *Chip8);

//I tried to minimize the amount of global variables as much as possible.
//However the scale factor, width, and height variables would break the program when included in the struct.

int main(int argc, char *argv[]) 
{
    //Initilize system.
    Chip8System Chip8;

    Chip8Init(&Chip8);
    //SDL initilization and window + Renderer creation
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED, Chip8.WIDTH, Chip8.HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    
    //Run code in loop,.
    while (true) {
        //Fetch Opcode
        uint16_t Opcode = Chip8.Chip8Memory[Chip8.PC] << 8 | Chip8.Chip8Memory[Chip8.PC + 1]; //Opcode is 2 bytes long, so we need to combine the two bytes into one 16 bit number.
        
        //Fetech Key Press
        Chip8Keyboard(&Chip8);

        //Decode and exectute Opcode
        Chip8CPU(&Chip8, Opcode);

        //Check if display needs to be updated.
        if (Chip8.DisplayUpdate) {
            Chip8DisplayOut(&Chip8, renderer);
        }

        Chip8UpdateTimers(&Chip8);
        Chip8.PC += 2; //Increment PC by 2, since each opcode is 2 bytes long.

        //Wait for 60 hz
        SDL_Delay(16); //Roughly 16 ms, 60 hz is 16.66 ms, but this is rather close.
    }
    return EXIT_SUCCESS;
};

void Chip8Init(Chip8System *Chip8) {
    //Local Variables
    std::string ROMName;
    //Set up scale factor system
    //This scale factor system enables users to implement whatever resolution requirements they have.
    std::cout << "Please enter the scale factor for the Chip-8 System." << std::endl << "Common Scale Factors are 10x for 640x320, 20x for 1280x640, 30x for 1920x960, 40x for 2560x1280, and 60x for 3840x1920." << std::endl << "Please note that this is what determines pixel size." << std::endl;
    std::cin >> Chip8->scalefactor;

    while (Chip8->scalefactor < 1) {
        std::cout << "Please enter a valid number greater than zero." << std::endl;
        std::cin >> Chip8->scalefactor;
    }

    Chip8->WIDTH = 64 * Chip8->scalefactor;
    Chip8->HEIGHT = 32 * Chip8->scalefactor;

    //Clear Memory, clear varaibles, clear stack, clear keypad, clear display, load fontset into memory, 
    //Clear Memory
    for (int i = 0; i < 4096; i++) {
        Chip8->Chip8Memory[i] = 0;
    }

    //Clear variables, stack, and keypad
    for (int j = 0; j < 16; j++) {
        Chip8->V[j] = 0;
        Chip8->Stack[j] = 0;
        Chip8->Chip8KeyPad[j] = 0;
    }

    //Clear Display
    for (int x = 0; x < 64; x++) {
        for (int y = 0; y < 32; y++) {
            Chip8->Chip8Display[x][y] = 0; //Set each pixel to off.
        }
    }

    //Load font set into memory location 0x50 to 0x9F, (Location 80 and 159)
    for (int f = 80; f < 160; f++) {
        Chip8->Chip8Memory[f] = Chip8->FONT[f-80];
    }

    //Get Rom File from User
    std::cout << "Please enter the file name of your ROM file." << std::endl << "Please include the .ch8 at the end! " << std::endl;
    std::cin >> ROMName;

    std::ifstream file(ROMName, std::ios::binary);

    if (!file.is_open()) {
        std::cout << "Unable to open ROM file, exiting...." << std::endl;
        exit(EXIT_FAILURE);
    }    

    file.seekg(0, std::ios::end);//Use to find length of the ROM, basically
    uint16_t ROMSIZE = file.tellg();//get the length of the rom (Max rom size is actually 3586, so only 12 bits are needed.)
    file.seekg(0, std::ios::beg); //Go back to beginning of file.

    //Check if rom is too big
    if (ROMSIZE > 3586) {
        std::cout << "ROM File too large to fit in memory, exiting...." << std::endl;
        file.close();
        exit(EXIT_FAILURE);
    }

    //Load rom into memory
    file.read(reinterpret_cast<char*>(Chip8->Chip8Memory) + 0x200, ROMSIZE);
    
    //Close Rom
    file.close();
}

void Chip8DisplayOut(Chip8System *Chip8, SDL_Renderer *renderer) {
    //Reset rendered image, so that the new frame doesn't overlap the old one;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); //accepts R, G, B, A in that order
    SDL_RenderClear(renderer);

    SDL_Rect DisplayOut[64][32];
    for (int a = 0; a < 64; a++) {
        for (int b = 0; b < 32; b++) {
            DisplayOut[a][b].x = a * Chip8->scalefactor; 
            DisplayOut[a][b].y = b * Chip8->scalefactor;
            DisplayOut[a][b].w = Chip8->scalefactor;
            DisplayOut[a][b].h = Chip8->scalefactor;
        }
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); //accepts R, G, B, A in that order
    //Render each tile
    for (int x = 0; x < 64; x++) {
        for (int y = 0; y < 32; y++) {
            switch (Chip8->Chip8Display[x][y]) {
                case 0:
                        break;
                case 1:
                        SDL_RenderFillRect(renderer, &DisplayOut[x][y]);
                        break;
            }
        }
    }
    SDL_RenderPresent(renderer);
    return;
}

void Chip8UpdateTimers(Chip8System *Chip8) {
    if (Chip8->DelayTimer > 0) { //Even though timers are supposed to update asynchonously, I don't know how to use multithreading.
        Chip8->DelayTimer--;
    }
    if (Chip8->SoundTimer > 0) {
        Chip8->SoundTimer--;
        Beep(1030,16); //16 ms, close enough to 60 hz
        //Play Sound for exactly one clock cycle.
        //When sound timer is 0, the sound will not play.
    }
    return;
}



void Chip8Keyboard(Chip8System *Chip8) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            exit(0);
        }
        if (event.type == SDL_KEYDOWN) {
            for (int i = 0; i < 16; i++) {
                if (event.key.keysym.sym == Chip8->keymap[i]) {
                    Chip8->Chip8KeyPad[i] = 1;
                }
            }
        }
        if (event.type == SDL_KEYUP) {
            for (int i = 0; i < 16; i++) {
                if (event.key.keysym.sym == Chip8->keymap[i]) {
                    Chip8->Chip8KeyPad[i] = 0;
                }
            }
        }
    }
    return;
}


void Chip8CPU(Chip8System *Chip8, uint16_t opcode) {
    switch (opcode & 0xF000) //Check the value of the first 4 bits
    {
        case 0x0000: 
            switch (opcode & 0x00FF)
            {
                case 0x00E0: //Clear Screen
                    for (int x = 0; x < 64; x++) {
                        for (int y = 0; y < 32; y++) {
                            Chip8->Chip8Display[x][y] = 0; //Set each pixel to off.
                        }
                    }
                    break;

                case 0x00EE: //Return from Subtroutine 
                    Chip8->PC = Chip8->Stack[Chip8->SP];
                    Chip8->SP++;
                    break;
            }
            break;

        case 0x1000: //Jump
            Chip8->PC = opcode & 0x0FFF; //Since our system increments PC anyways, we need to subtract by two.
            Chip8->PC -= 2;
            break;

        case 0x2000: //Call subroutine
            Chip8->SP--;
            Chip8->Stack[Chip8->SP] = Chip8->PC;

            //Move to PC to subroutine.
            Chip8->PC = opcode & 0x0FFF; //Since our system increments PC anyways, we need to subtract by two.
            Chip8->PC -= 2;
            break; 
        
        case 0x3000: //Skip next instruction depending on Vx == kk
            if (Chip8->V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
                Chip8->PC += 2;
            }
            break; 
        
        case 0x4000: //Skip next instruction depeding on Vx != kk
            if (Chip8->V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
                Chip8->PC += 2;
            }
            break; 
        
        case 0x5000: //Skip next instruction depending on Vx == Vy
            if (Chip8->V[(opcode & 0x0F00) >> 8] == Chip8->V[(opcode & 0x00F0) >> 4]) {
                Chip8->PC += 2;
            }
            break; 

        case 0x6000: //Load Vx with kk
            Chip8->V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            break;

        case 0x7000: //Add Vx to Vx + kk
            Chip8->V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            break;

        case 0x8000: //LOTS of Math operations
            switch (opcode & 0x000F) //Check the value of the last 4 bits
            {
                case 0x0000: //LD Vx, Vy
                {
                    Chip8->V[(opcode & 0x0F00) >> 8] = Chip8->V[(opcode & 0x00F0) >> 4];
                    break;
                }
                case 0x0001: //OR Vx, Vy 
                {
                    Chip8->V[(opcode & 0x0F00) >> 8] |= Chip8->V[(opcode & 0x00F0) >> 4];
                    break;
                }
                case 0x0002: //AND Vx, Vy
                {
                    Chip8->V[(opcode & 0x0F00) >> 8] &= Chip8->V[(opcode & 0x00F0) >> 4];
                    break;
                }
                case 0x0003: //XOR Vx, Vy
                {
                    Chip8->V[(opcode & 0x0F00) >> 8] ^= Chip8->V[(opcode & 0x00F0) >> 4];
                    break;
                }
                case 0x0004: //ADD Vx, Vy
                {
                    uint8_t XAddval = Chip8->V[(opcode & 0x0F00) >> 8];
                    uint8_t YAddval = Chip8->V[(opcode & 0x00F0) >> 4];
                    

                    if ((Chip8->V[(opcode & 0x0F00) >> 8] + Chip8->V[(opcode & 0x00F0) >> 4]) > 255) {
                        Chip8->V[15] = 1;
                    }
                    else {
                        Chip8->V[15] = 0;
                    }

                    XAddval += YAddval; //Actual 8 bit math
                    
                    Chip8->V[(opcode & 0x0F00) >> 8] = XAddval;

                    break;
                }
                case 0x0005:  //Sub Vx, Vy 
                {
                    if (Chip8->V[(opcode & 0x0F00) >> 8] > Chip8->V[(opcode & 0x00F0) >> 4]) {
                        Chip8->V[15] = 1;
                    }
                    else {
                        Chip8->V[15] = 0;
                    }

                    Chip8->V[(opcode & 0x0F00) >> 8] -= Chip8->V[(opcode & 0x00F0) >> 4];
                    break;
                }
                case 0x0006: //Shift Vx right
                {
                    Chip8->V[15] = Chip8->V[(opcode & 0x0F00) >> 8] & 0x1;
                    Chip8->V[(opcode & 0x0F00) >> 8] >>= 1;
                    break;
                }
                case 0x0007: //Sub Vy, Vx
                {
                    if (Chip8->V[(opcode & 0x0F00) >> 8] < Chip8->V[(opcode & 0x00F0) >> 4]) {
                        Chip8->V[15] = 1;
                    }
                    else {
                        Chip8->V[15] = 0;
                    }
                    Chip8->V[(opcode & 0x00F0) >> 4] -= Chip8->V[(opcode & 0x0F00) >> 8];
                    break;
                }
                case 0x000E: //Shift Vx left  
                {
                    Chip8->V[15] = Chip8->V[(opcode & 0x0F00) >> 8] & 0x80;
                    Chip8->V[(opcode & 0x0F00) >> 8] <<= 1;
                    break;
                }
            }
            break;  
        case 0x9000: //Skip  next instruction if Vx != Vy
            if (Chip8->V[(opcode & 0x0F00) >> 8] != Chip8->V[(opcode & 0x00F0) >> 4]) {
                Chip8->PC += 2;
            }
            break; 


        case 0xA000: //Load I with nnn
            Chip8->I = opcode & 0x0FFF;
            break;

        case 0xB000: //Jump to location nnn + V0
            Chip8->PC = (opcode & 0x0FFF) + Chip8->V[0];
            break; 

        case 0xC000: //Load Vx with random number and kk
            Chip8->V[(opcode & 0x0F00) >> 8] = (rand() % 256) & (opcode & 0x00FF);
            break; 

        case 0xD000: //Draw sprite 
        {
            //This Opcode was very complicated to implement, so I used these resources to help me out.
            //https://tobiasvl.github.io/blog/write-a-chip-8-emulator/
            //https://github.com/cookerlyk/Chip8/tree/master

            //Get values from Opcode
            uint8_t Spritex = Chip8->V[(opcode & 0x0F00) >> 8] % 64;
            uint8_t Spritey = Chip8->V[(opcode & 0x00F0) >> 4] % 32; //Get values from the registers
            uint8_t spriteheight = opcode & 0x000F; //Get sprite height
            uint8_t spritepixel; 

            //Set Vf to 0
            Chip8->V[15] = 0;

            //Drawing Loops
            for (int ylen = 0; ylen < spriteheight; ylen++) {

                spritepixel = Chip8->Chip8Memory[Chip8->I + ylen]; //The current pixel being drawn, given to us from the I value and the sprite height.
    
                for (int xlen = 0; xlen < 8; xlen++) { //X val is preset always to 8, since the sprite is 8 pixels wide.
                    
                    //Check if the sprite data has this pixel set to 1
                    if ((spritepixel & (0x80 >> xlen)) != 0) {
                        
                        if (Chip8->Chip8Display[(Spritex + xlen)][(Spritey + ylen)] == 1) { 
                            //If there is a colision, where a pixel is alreay on, set it to zero and set the colision flag to 1.
                            Chip8->V[15] = 1; //Set colision value equal to 1
                        }

                        Chip8->Chip8Display[(Spritex + xlen)][(Spritey + ylen)] ^= 1; //Exclusive OR the pixel
                    }
                }
            }

            //End 
            Chip8->DisplayUpdate = 1;
            break;
        }

        case 0xE000: //Skip next instruction if key is pressed or not pressed
            switch (opcode & 0x00FF)
            {
                case 0x009E: //Skip instruction if key is pressed
                
                    if (Chip8->Chip8KeyPad[Chip8->V[(opcode & 0x0F00) >> 8]] != 0) {
                        Chip8->PC += 2;
                    }
                    break;

                case 0x00A1: //Skip instruction if key is not pressed
                    if (Chip8->Chip8KeyPad[Chip8->V[(opcode & 0x0F00) >> 8]] == 0) {
                        Chip8->PC += 2;
                    }
                    break;
            }
            break; 

        case 0xF000: //Lots of loading operations
            switch (opcode & 0x00FF) 
            {
                case 0x0007: //Set Vx to Delay Timer
                    Chip8->V[(opcode & 0x0F00) >> 8] = Chip8->DelayTimer;
                    break;
                
                case 0x000A: //Wait for keypress then store in Vx 
                {
                    for (int i = 0; i < 16; i++) {
                        if (Chip8->Chip8KeyPad[i] != 0) {
                            Chip8->V[(opcode & 0x0F00) >> 8] = i;
                            break;
                        }
                    }

                    Chip8->PC -= 2; //Since we are waiting for a keypress, we need to decrement the PC by 2. 
                    //This will ensure a loop will occur until we get a keypress.
                    return;
                }
                case 0x0015: //Set delay timer to Vx
                    Chip8->DelayTimer = Chip8->V[(opcode & 0x0F00) >> 8];
                    break;
                
                case 0x0018: //Set sound timer to Vc
                    Chip8->SoundTimer = Chip8->V[(opcode & 0x0F00) >> 8];
                    break;
                
                case 0x001E: //Set I equal to I + Vx
                    Chip8->I += Chip8->V[(opcode & 0x0F00) >> 8];
                    break;
                
                case 0x0029: //Set I equal to location of the sprite for Digit at Vx
                    Chip8->I = Chip8->V[(opcode & 0x0F00) >> 8] * 5; 
                    break;
                
                case 0x0033: //Store the decimal representation of Vx in memory locations I, I+1, and I+2
                    Chip8->Chip8Memory[Chip8->I] = Chip8->V[(opcode & 0x0F00) >> 8] / 100;
                    Chip8->Chip8Memory[Chip8->I + 1] = (Chip8->V[(opcode & 0x0F00) >> 8] / 10) % 10;
                    Chip8->Chip8Memory[Chip8->I + 2] = Chip8->V[(opcode & 0x0F00) >> 8] % 10;
                    break;
                
                case 0x0055: //Store V0 to Vx in memory at location I and up 
                {
                    uint8_t numRegStore = (opcode & 0x0F00) >> 8;
                    for (int i = 0; i <= numRegStore; i++) {
                        Chip8->Chip8Memory[Chip8->I + i] = Chip8->V[i];
                    }
                    break;
                }
                case 0x0065: //Load V0 to Vx from memory at location I and up 
                {
                    uint8_t numRegLoad = (opcode & 0x0F00) >> 8;
                    for (int i = 0; i <= numRegLoad; i++) {
                        Chip8->V[i] = Chip8->Chip8Memory[Chip8->I + i];
                    }
                    break;
                }
            }
        break; 
    }
    return;
}