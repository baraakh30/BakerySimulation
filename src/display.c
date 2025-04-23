#include "../include/display.h"
#include "../include/shared.h"
#include "../include/config.h"
#include <GL/glut.h>
#include <math.h>

// Global variables for display
int window_width = 1200;
int window_height = 800;
int refresh_rate = 100; // milliseconds

// Color definitions
typedef struct
{
    float r, g, b;
} Color;

const Color COLOR_BACKGROUND = {0.95f, 0.95f, 0.95f};
const Color COLOR_TEXT = {0.1f, 0.1f, 0.1f};
const Color COLOR_BREAD = {0.76f, 0.60f, 0.42f};
const Color COLOR_CAKE = {0.93f, 0.78f, 0.85f};
const Color COLOR_SANDWICH = {0.85f, 0.72f, 0.55f};
const Color COLOR_SWEETS = {0.95f, 0.70f, 0.70f};
const Color COLOR_SWEET_PATISSERIE = {0.98f, 0.86f, 0.68f};
const Color COLOR_SAVORY_PATISSERIE = {0.82f, 0.85f, 0.65f};
const Color COLOR_SUPPLY = {0.65f, 0.82f, 0.90f};
const Color COLOR_PROFIT = {0.50f, 0.80f, 0.50f};
const Color COLOR_COMPLAINTS = {0.90f, 0.50f, 0.50f};

// Function to display text
void draw_text(float x, float y, const char *string)
{
    glColor3f(COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b);
    glRasterPos2f(x, y);

    for (const char *c = string; *c != '\0'; c++)
    {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
    }
}

// Initialize OpenGL display
void init_display(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(window_width, window_height);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Bakery Management Simulation");

    glClearColor(COLOR_BACKGROUND.r, COLOR_BACKGROUND.g, COLOR_BACKGROUND.b, 1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, window_width, 0, window_height);

    glutDisplayFunc(display_function);
    glutReshapeFunc(reshape_function);
    glutKeyboardFunc(keyboard_function);
    glutTimerFunc(refresh_rate, timer_function, 0);

    glutMainLoop();
}

// Display callback function
void display_function(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw bakery state
    draw_bakery_state();

    // Draw inventory
    draw_inventory();

    // Draw supplies
    draw_supplies();

    // Draw statistics
    draw_statistics();

    glutSwapBuffers();
}

// Reshape callback function
void reshape_function(int width, int height)
{
    window_width = width;
    window_height = height;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);

    glMatrixMode(GL_MODELVIEW);
}

// Timer callback function for periodic updates
void timer_function(int value)
{
    glutPostRedisplay();
    glutTimerFunc(refresh_rate, timer_function, 0);
}

// Keyboard callback function
void keyboard_function(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 27: // Escape key
        exit(0);
        break;
    case 'q':
    case 'Q':
        exit(0);
        break;
    }
}

// Update display with current bakery state
void update_display(void)
{
    glutPostRedisplay();
}

// Draw bakery state information
void draw_bakery_state(void)
{
    sem_lock(0);

    char buffer[100];
    float y_pos = window_height - 30;

    // Title
    glColor3f(0.2f, 0.2f, 0.6f);
    sprintf(buffer, "BAKERY SIMULATION STATUS");
    draw_text(window_width / 2 - 120, y_pos, buffer);
    y_pos -= 30;

    // Running time
    time_t current_time = time(NULL);
    int elapsed_seconds = current_time - bakery_state->start_time;
    int hours = elapsed_seconds / 3600;
    int minutes = (elapsed_seconds % 3600) / 60;
    int seconds = elapsed_seconds % 60;

    sprintf(buffer, "Running time: %02d:%02d:%02d", hours, minutes, seconds);
    draw_text(20, y_pos, buffer);
    y_pos -= 20;

    // Daily profit
    sprintf(buffer, "Daily profit: $%.2f / $%.2f (%.1f%%)",
            bakery_state->daily_profit,
            bakery_state->profit_threshold,
            bakery_state->daily_profit * 100.0 / bakery_state->profit_threshold);
    draw_text(20, y_pos, buffer);
    y_pos -= 20;

    // Complaints
    sprintf(buffer, "Complaints: %d / %d",
            bakery_state->customer_complaints,
            bakery_state->max_complaints);
    draw_text(20, y_pos, buffer);
    y_pos -= 20;

    // Frustrated customers
    sprintf(buffer, "Frustrated customers: %d / %d",
            bakery_state->frustrated_customers,
            bakery_state->max_frustrated_customers);
    draw_text(20, y_pos, buffer);
    y_pos -= 20;

    // Missing items requests
    sprintf(buffer, "Missing items requests: %d / %d",
            bakery_state->missing_items_requests,
            bakery_state->max_missing_items_requests);
    draw_text(20, y_pos, buffer);
    y_pos -= 20;

    // Staff distribution
    y_pos -= 20;
    draw_text(20, y_pos, "Staff Distribution:");
    y_pos -= 20;

    // Chefs
    for (int team = TEAM_PASTE; team <= TEAM_BREAD; team++)
    {
        char team_name[30];
        switch (team)
        {
        case TEAM_PASTE:
            strcpy(team_name, "Paste");
            break;
        case TEAM_CAKE:
            strcpy(team_name, "Cake");
            break;
        case TEAM_SANDWICH:
            strcpy(team_name, "Sandwich");
            break;
        case TEAM_SWEETS:
            strcpy(team_name, "Sweets");
            break;
        case TEAM_SWEET_PATISSERIE:
            strcpy(team_name, "Sweet Patisserie");
            break;
        case TEAM_SAVORY_PATISSERIE:
            strcpy(team_name, "Savory Patisserie");
            break;
        case TEAM_BREAD:
            strcpy(team_name, "Bread");
            break;
        default:
            strcpy(team_name, "Unknown");
            break;
        }

        sprintf(buffer, "  - %s Team: %d chefs", team_name, bakery_state->chefs_per_team[team]);
        draw_text(20, y_pos, buffer);
        y_pos -= 20;
    }

    // Bakers
    for (int team = TEAM_BAKE_CAKES_SWEETS; team <= TEAM_BAKE_BREAD; team++)
    {
        char team_name[30];
        switch (team)
        {
        case TEAM_BAKE_CAKES_SWEETS:
            strcpy(team_name, "Cakes & Sweets");
            break;
        case TEAM_BAKE_PATISSERIES:
            strcpy(team_name, "Patisseries");
            break;
        case TEAM_BAKE_BREAD:
            strcpy(team_name, "Bread");
            break;
        default:
            strcpy(team_name, "Unknown");
            break;
        }

        sprintf(buffer, "  - %s Baking Team: %d bakers", team_name, bakery_state->bakers_per_team[team]);
        draw_text(20, y_pos, buffer);
        y_pos -= 20;
    }

    // Other staff
    sprintf(buffer, "  - Supply chain: %d employees", bakery_state->supply_employees);
    draw_text(20, y_pos, buffer);
    y_pos -= 20;

    sprintf(buffer, "  - Sellers: %d employees", bakery_state->sellers);
    draw_text(20, y_pos, buffer);

    sem_unlock(0);
}

// Draw inventory information with bar graphs
void draw_inventory(void)
{
    sem_lock(0);

    const float bar_max_width = 150.0f;
    const float bar_height = 20.0f;
    const float start_x = window_width - 200;
    float y_pos = window_height - 30;

    // Title
    glColor3f(0.2f, 0.6f, 0.2f);
    draw_text(start_x, y_pos, "INVENTORY");
    y_pos -= 30;

    // Draw inventory bars for each item type
    const char *item_names[] = {
        "Bread", "Cake", "Sandwich", "Sweets",
        "Sweet Patisserie", "Savory Patisserie", "Paste"};

    const Color item_colors[] = {
        COLOR_BREAD, COLOR_CAKE, COLOR_SANDWICH, COLOR_SWEETS, COLOR_SWEET_PATISSERIE, COLOR_SAVORY_PATISSERIE, {0.8f, 0.8f, 0.8f}};

    int max_inventory = 1; // To avoid division by zero
    for (int i = 0; i < ITEM_COUNT; i++)
    {
        int item_total = 0;
        for (int j = 0; j < 100; j++)
        {
            item_total += bakery_state->inventory[i][j];
        }
        if (item_total > max_inventory)
        {
            max_inventory = item_total;
        }
    }

    for (int i = 0; i < ITEM_COUNT; i++)
    {
        int item_total = 0;
        for (int j = 0; j < 100; j++)
        {
            item_total += bakery_state->inventory[i][j];
        }

        // Draw label
        draw_text(start_x - 180, y_pos, item_names[i]);

        // Draw bar
        glColor3f(item_colors[i].r, item_colors[i].g, item_colors[i].b);
        float bar_width = (float)item_total * bar_max_width / max_inventory;
        glBegin(GL_QUADS);
        glVertex2f(start_x, y_pos - bar_height + 5);
        glVertex2f(start_x + bar_width, y_pos - bar_height + 5);
        glVertex2f(start_x + bar_width, y_pos + 5);
        glVertex2f(start_x, y_pos + 5);
        glEnd();

        // Draw count
        char buffer[20];
        sprintf(buffer, "%d", item_total);
        glColor3f(COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b);
        draw_text(start_x + bar_width + 10, y_pos - 5, buffer);

        y_pos -= 30;
    }

    sem_unlock(0);
}

// Draw supplies information with bar graphs
void draw_supplies(void)
{
    sem_lock(0);

    const float bar_max_width = 150.0f;
    const float bar_height = 20.0f;
    const float start_x = window_width - 200;
    float y_pos = window_height / 2;

    // Title
    glColor3f(0.2f, 0.2f, 0.6f);
    draw_text(start_x, y_pos, "SUPPLIES");
    y_pos -= 30;

    // Draw supply bars for each supply type
    const char *supply_names[] = {
        "Wheat", "Yeast", "Butter", "Milk",
        "Sugar/Salt", "Sweet Items", "Cheese/Salami"};

    int max_supply = 1; // To avoid division by zero
    for (int i = 0; i < SUPPLY_COUNT; i++)
    {
        if (bakery_state->supplies[i] > max_supply)
        {
            max_supply = bakery_state->supplies[i];
        }
    }

    for (int i = 0; i < SUPPLY_COUNT; i++)
    {
        // Draw label
        draw_text(start_x - 180, y_pos, supply_names[i]);

        // Draw bar
        glColor3f(COLOR_SUPPLY.r, COLOR_SUPPLY.g, COLOR_SUPPLY.b);
        float bar_width = (float)bakery_state->supplies[i] * bar_max_width / max_supply;
        glBegin(GL_QUADS);
        glVertex2f(start_x, y_pos - bar_height + 5);
        glVertex2f(start_x + bar_width, y_pos - bar_height + 5);
        glVertex2f(start_x + bar_width, y_pos + 5);
        glVertex2f(start_x, y_pos + 5);
        glEnd();

        // Draw count
        char buffer[20];
        sprintf(buffer, "%d", bakery_state->supplies[i]);
        glColor3f(COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b);
        draw_text(start_x + bar_width + 10, y_pos - 5, buffer);

        y_pos -= 30;
    }

    sem_unlock(0);
}

// Draw statistics charts
void draw_statistics(void)
{
    sem_lock(0);

    const float chart_width = 300.0f;
    const float chart_height = 150.0f;
    const float start_x = 20;
    float y_pos = window_height / 2;

    // Title
    glColor3f(0.5f, 0.35f, 0.05f);
    draw_text(start_x, y_pos, "STATISTICS");
    y_pos -= 30;

    // Draw production vs sales chart
    draw_text(start_x, y_pos, "Production vs Sales:");
    y_pos -= 20;

    int max_value = 1; // To avoid division by zero
    for (int i = 0; i < ITEM_COUNT; i++)
    {
        if (bakery_state->items_produced[i] > max_value)
        {
            max_value = bakery_state->items_produced[i];
        }
        if (bakery_state->items_sold[i] > max_value)
        {
            max_value = bakery_state->items_sold[i];
        }
    }

    // Draw chart background
    glColor3f(0.95f, 0.95f, 0.95f);
    glBegin(GL_QUADS);
    glVertex2f(start_x, y_pos - chart_height);
    glVertex2f(start_x + chart_width, y_pos - chart_height);
    glVertex2f(start_x + chart_width, y_pos);
    glVertex2f(start_x, y_pos);
    glEnd();

    // Draw chart border
    glColor3f(0.7f, 0.7f, 0.7f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(start_x, y_pos - chart_height);
    glVertex2f(start_x + chart_width, y_pos - chart_height);
    glVertex2f(start_x + chart_width, y_pos);
    glVertex2f(start_x, y_pos);
    glEnd();

    // Draw production bars
    const float bar_width = chart_width / (ITEM_COUNT * 2 + 1);
    for (int i = 0; i < ITEM_COUNT; i++)
    {
        float bar_height = (float)bakery_state->items_produced[i] * chart_height / max_value;

        // Production bar
        glColor3f(0.4f, 0.7f, 0.4f);
        glBegin(GL_QUADS);
        glVertex2f(start_x + bar_width * (i * 2 + 1), y_pos - chart_height);
        glVertex2f(start_x + bar_width * (i * 2 + 2), y_pos - chart_height);
        glVertex2f(start_x + bar_width * (i * 2 + 2), y_pos - chart_height + bar_height);
        glVertex2f(start_x + bar_width * (i * 2 + 1), y_pos - chart_height + bar_height);
        glEnd();

        // Sales bar
        bar_height = (float)bakery_state->items_sold[i] * chart_height / max_value;
        glColor3f(0.7f, 0.4f, 0.4f);
        glBegin(GL_QUADS);
        glVertex2f(start_x + bar_width * (i * 2 + 2), y_pos - chart_height);
        glVertex2f(start_x + bar_width * (i * 2 + 3), y_pos - chart_height);
        glVertex2f(start_x + bar_width * (i * 2 + 3), y_pos - chart_height + bar_height);
        glVertex2f(start_x + bar_width * (i * 2 + 2), y_pos - chart_height + bar_height);
        glEnd();

        // Item label
        char label[2];
        sprintf(label, "%d", i);
        draw_text(start_x + bar_width * (i * 2 + 1.5), y_pos - chart_height - 15, label);
    }

    // Legend
    glColor3f(0.4f, 0.7f, 0.4f);
    glBegin(GL_QUADS);
    glVertex2f(start_x, y_pos - chart_height - 30);
    glVertex2f(start_x + 15, y_pos - chart_height - 30);
    glVertex2f(start_x + 15, y_pos - chart_height - 20);
    glVertex2f(start_x, y_pos - chart_height - 20);
    glEnd();
    draw_text(start_x + 20, y_pos - chart_height - 25, "Production");

    glColor3f(0.7f, 0.4f, 0.4f);
    glBegin(GL_QUADS);
    glVertex2f(start_x + 100, y_pos - chart_height - 30);
    glVertex2f(start_x + 115, y_pos - chart_height - 30);
    glVertex2f(start_x + 115, y_pos - chart_height - 20);
    glVertex2f(start_x + 100, y_pos - chart_height - 20);
    glEnd();
    draw_text(start_x + 120, y_pos - chart_height - 25, "Sales");

    // Customer statistics
    y_pos = y_pos - chart_height - 60;
    draw_text(start_x, y_pos, "Customer Statistics:");
    y_pos -= 20;

    char buffer[100];
    sprintf(buffer, "Customers served: %d", bakery_state->customers_served);
    draw_text(start_x, y_pos, buffer);
    y_pos -= 20;

    sprintf(buffer, "Waiting customers: %d", bakery_state->waiting_customers);
    draw_text(start_x, y_pos, buffer);
    y_pos -= 20;

    // Profit chart (simplified line chart)
    y_pos -= 20;
    draw_text(start_x, y_pos, "Daily Profit Progress:");
    y_pos -= 20;

    // Draw profit bar
    float progress = bakery_state->daily_profit / bakery_state->profit_threshold;
    if (progress > 1.0f)
        progress = 1.0f;

    glColor3f(COLOR_PROFIT.r, COLOR_PROFIT.g, COLOR_PROFIT.b);
    glBegin(GL_QUADS);
    glVertex2f(start_x, y_pos - 20);
    glVertex2f(start_x + chart_width * progress, y_pos - 20);
    glVertex2f(start_x + chart_width * progress, y_pos);
    glVertex2f(start_x, y_pos);
    glEnd();

    glColor3f(0.7f, 0.7f, 0.7f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(start_x, y_pos - 20);
    glVertex2f(start_x + chart_width, y_pos - 20);
    glVertex2f(start_x + chart_width, y_pos);
    glVertex2f(start_x, y_pos);
    glEnd();

    sprintf(buffer, "$%.2f / $%.2f (%.1f%%)",
            bakery_state->daily_profit,
            bakery_state->profit_threshold,
            progress * 100.0f);
    draw_text(start_x + chart_width / 2 - 50, y_pos - 15, buffer);

    sem_unlock(0);
}