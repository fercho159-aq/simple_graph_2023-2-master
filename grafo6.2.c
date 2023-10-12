/*Copyright (C) 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * 2023 - francisco dot rodriguez at ingenieria dot unam dot mx
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#include "List.h"

// 29/03/23:
// Esta versión no borra elementos
// Esta versión no modifica los datos originales

#ifndef DBG_HELP
#define DBG_HELP 0
#endif  

#if DBG_HELP > 0
#define DBG_PRINT( ... ) do{ fprintf( stderr, "DBG:" __VA_ARGS__ ); } while( 0 )
#else
#define DBG_PRINT( ... ) ;
#endif  


// Aunque en este ejemplo estamos usando tipos básicos, vamos a usar al alias |Item| para resaltar
// aquellos lugares donde estamos hablando de DATOS y no de índices.
typedef int Item;


//----------------------------------------------------------------------
//                           Vertex stuff: 
//----------------------------------------------------------------------

/**
 * @brief Colores para el algoritmo BFS
 */
typedef enum
{
    BLACK, ///< vértice no descubierto
    GRAY,  ///< vértice descubierto
    WHITE  ///< vértice visitado
} eGraphColors;

// Estructura para la información de un aeropuerto
typedef struct
{
    int id;
    char iata_code[4];
    char country[65];
    char city[65];
    char name[65];
    int utc_time;
} Airport;

typedef struct
{
    Item data; 
    List* neighbors;
    eGraphColors color;
    int distance;
    int predecessor;
    Airport airport_info;
} Vertex;





/**
 * @brief Hace que cursor libre apunte al inicio de la lista de vecinos. Se debe
 * de llamar siempre que se vaya a iniciar un recorrido de dicha lista.
 *
 * @param v El vértice de trabajo (es decir, el vértice del cual queremos obtener 
 * la lista de vecinos).
 */
void Vertex_Start( Vertex* v )
{
   assert( v );

   List_Cursor_front( v->neighbors );
}

/**
 * @brief Mueve al cursor libre un nodo adelante.
 *
 * @param v El vértice de trabajo.
 *
 * @pre El cursor apunta a un nodo válido.
 * @post El cursor se movió un elemento a la derecha en la lista de vecinos.
 */
void Vertex_Next( Vertex* v )
{
   List_Cursor_next( v->neighbors );
}

/**
 * @brief Indica si se alcanzó el final de la lista de vecinos.
 *
 * @param v El vértice de trabajo.
 *
 * @return true si se alcanazó el final de la lista; false en cualquier otro
 * caso.
 */
bool Vertex_End( const Vertex* v )
{
   return List_Cursor_end( v->neighbors );
}


/**
 * @brief Devuelve el índice del vecino al que apunta actualmente el cursor en la lista de vecinos
 * del vértice |v|.
 *
 * @param v El vértice de trabajo (del cual queremos conocer el índice de su vecino).
 *
 * @return El índice del vecino en la lista de vértices.
 *
 * @pre El cursor debe apuntar a un nodo válido en la lista de vecinos.
 *
 * Ejemplo
 * @code
   Vertex* v = Graph_GetVertexByKey( grafo, 100 );
   for( Vertex_Start( v ); !Vertex_End( v ); Vertex_Next( v ) )
   {
      int index = Vertex_GetNeighborIndex( v );

      Item val = Graph_GetDataByIndex( g, index );

      // ...
   }
   @endcode
   @note Esta función debe utilizarse únicamente cuando se recorra el grafo con las funciones 
   Vertex_Start(), Vertex_End() y Vertex_Next().
 */
Data Vertex_GetNeighborIndex( const Vertex* v )
{
   return List_Cursor_get( v->neighbors );
}


//----------------------------------------------------------------------
//                           Graph stuff: 
//----------------------------------------------------------------------

/** Tipo del grafo.
 */
typedef enum 
{ 
   eGraphType_UNDIRECTED, ///< grafo no dirigido
   eGraphType_DIRECTED    ///< grafo dirigido (digraph)
} eGraphType; 

/**
 * @brief Declara lo que es un grafo.
 */
typedef struct
{
   Vertex* vertices; ///< Lista de vértices
   int size;      ///< Tamaño de la lista de vértices

   /**
    * Número de vértices actualmente en el grafo. 
    * Como esta versión no borra vértices, lo podemos usar como índice en la
    * función de inserción
    */
   int len;  

   eGraphType type; ///< tipo del grafo, UNDIRECTED o DIRECTED
} Graph;

//----------------------------------------------------------------------
//                     Funciones privadas
//----------------------------------------------------------------------

// vertices: lista de vértices
// size: número de elementos en la lista de vértices
// key: valor a buscar
// ret: el índice donde está la primer coincidencia; -1 si no se encontró
static int find( Vertex vertices[], int size, int key )
{
   for( int i = 0; i < size; ++i )
   {
      if( vertices[ i ].data == key ) return i;
   }

   return -1;
}

// busca en la lista de vecinos si el índice del vértice vecino ya se encuentra ahí
static bool find_neighbor( Vertex* v, int index )
{
   if( v->neighbors )
   {
      return List_Find( v->neighbors, index );
   }
   return false;
}

// vertex: vértice de trabajo
// index: índice en la lista de vértices del vértice vecino que está por insertarse
static void insert( Vertex* vertex, int index, float weigth )
{
   // crear la lista si no existe!
   
   if( !vertex->neighbors )
   {
      vertex->neighbors = List_New();
   }

   if( vertex->neighbors && !find_neighbor( vertex, index ) )
   {
      List_Push_back( vertex->neighbors, index, weigth );

      DBG_PRINT( "insert():Inserting the neighbor with idx:%d\n", index );
   } 
   else DBG_PRINT( "insert: duplicated index\n" );
}



//----------------------------------------------------------------------
//                     Funciones públicas
//----------------------------------------------------------------------


/**
 * @brief Crea un nuevo grafo.
 *
 * @param size Número de vértices que tendrá el grafo. Este valor no se puede
 * cambiar luego de haberlo creado.
 *
 * @return Un nuevo grafo.
 *
 * @pre El número de elementos es mayor que 0.
 */
Graph* Graph_New( int size, eGraphType type )
{
   assert( size > 0 );

   Graph* g = (Graph*) malloc( sizeof( Graph ) );
   if( g )
   {
      g->size = size;
      g->len = 0;
      g->type = type;

      g->vertices = (Vertex*) calloc( size, sizeof( Vertex ) );

      if( !g->vertices )
      {
         free( g );
         g = NULL;
      }
   }

   return g;
   // el cliente es responsable de verificar que el grafo se haya creado correctamente
}

void Graph_Delete( Graph** g )
{
   assert( *g );

   Graph* graph = *g;
   // para simplificar la notación 

   for( int i = 0; i < graph->size; ++i )
   {
      Vertex* vertex = &graph->vertices[ i ];
      // para simplificar la notación. 
      // La variable |vertex| sólo existe dentro de este for.

      if( vertex->neighbors )
      {
         List_Delete( &(vertex->neighbors) );
      }
   }

   free( graph->vertices );
   free( graph );
   *g = NULL;
}

void Graph_Print(Graph* g, int depth)
{
    for (int i = 0; i < g->len; ++i)
    {
        Vertex* vertex = &g->vertices[i];

        printf("Vertex %d - Color: %d, Distance: %d\n", vertex->airport_info.id, vertex->color, vertex->distance);

        printf("Airport Info:\n");
        printf("ID: %d\n", vertex->airport_info.id);
        printf("IATA Code: %s\n", vertex->airport_info.iata_code);
        printf("Country: %s\n", vertex->airport_info.country);
        printf("City: %s\n", vertex->airport_info.city);
        printf("Name: %s\n", vertex->airport_info.name);
        printf("UTC Time: %d\n", vertex->airport_info.utc_time);

       if (vertex->neighbors)
       {
        printf("Códigos IATA de los vecinos: ");
        for (List_Cursor_front(vertex->neighbors); !List_Cursor_end(vertex->neighbors);
        List_Cursor_next(vertex->neighbors))
             {
          Data d = List_Cursor_get(vertex->neighbors);
          int neighborIndex = d.index;
          printf("%s(W:%.2f) ", g->vertices[neighborIndex].airport_info.iata_code, d.weight);
          }
    printf("\n");
}
    }
    printf("\n");
}

/**
 * @brief Obtiene el peso de la arista entre los vértices |start| y |finish| en el grafo.
 *
 * @param g      El grafo.
 * @param start  Vértice de salida (el dato)
 * @param finish Vértice de llegada (el dato)
 *
 * @return El peso de la arista si existe, -1.0 si la arista no existe.
 */
double Graph_GetWeight( Graph* g, int start, int finish )
{
   assert( g->len > 0 );

   // Obtenemos los índices correspondientes:
   int start_idx = find( g->vertices, g->size, start );
   int finish_idx = find( g->vertices, g->size, finish );

   if( start_idx == -1 || finish_idx == -1 ) return -1.0;
   // Uno o ambos vértices no existen

   Vertex* vertex = &g->vertices[ start_idx ];
   if (vertex->neighbors) {
      for (List_Cursor_front(vertex->neighbors);
           !List_Cursor_end(vertex->neighbors);
           List_Cursor_next(vertex->neighbors)) {
         Data d = List_Cursor_get(vertex->neighbors);
         if (d.index == finish_idx) {
            return d.weight;
         }
      }
   }

   return -1;
}


/**
 * @brief Crea un vértice a partir de los datos de un aeropuerto.
 *
 * @param g      El grafo.
 * @param airport La información del aeropuerto.
 */
void Graph_AddVertex(Graph* g, Airport airport)
{
    assert(g->len < g->size);

    Vertex* vertex = &g->vertices[g->len];

    // Inicializa los campos del vértice
    vertex->data = g->len; // Puedes usar el índice como identificador
    vertex->neighbors = NULL;
    vertex->color = BLACK; // Inicializa el color a BLACK
    vertex->distance = 0;  // Inicializa la distancia a 0
    vertex->predecessor = -1; // Inicializa el predecesor a -1
    vertex->airport_info = airport; // Copia la información del aeropuerto

    ++g->len;
}

int Graph_GetSize( Graph* g )
{
   return g->size;
}

void Vertex_SetColor( Vertex* v, eGraphColors color )
{
    v->color = color;
}

eGraphColors Vertex_GetColor( Vertex* v )
{
    return v->color;
}

void Vertex_SetDistance( Vertex* v, int distance )
{
    v->distance = distance;
}

int Vertex_GetDistance( Vertex* v )
{
    return v->distance;
}

void Vertex_SetPredecessor( Vertex* v, int predecessor_idx )
{
    v->predecessor = predecessor_idx;
}

int Vertex_GetPredecessor( Vertex* v )
{
    return v->predecessor;
}



/**
 * @brief Inserta una relación de adyacencia del vértice |start| hacia el vértice |finish|.
 *
 * @param g      El grafo.
 * @param start  Vértice de salida (el dato)
 * @param finish Vertice de llegada (el dato)
 *
 * @return false si uno o ambos vértices no existen; true si la relación se creó con éxito.
 *
 * @pre El grafo no puede estar vacío.
 */
bool Graph_AddEdge( Graph* g, int start, int finish )
{
   assert( g->len > 0 );

   // obtenemos los índices correspondientes:
   int start_idx = find( g->vertices, g->size, start );
   int finish_idx = find( g->vertices, g->size, finish );

   DBG_PRINT( "AddEdge(): from:%d (with index:%d), to:%d (with index:%d)\n", start, start_idx, finish, finish_idx );

   if( start_idx == -1 || finish_idx == -1 ) return false;
   // uno o ambos vértices no existen

   insert( &g->vertices[ start_idx ], finish_idx, 0.0 );
   // insertamos la arista start-finish

   if( g->type == eGraphType_UNDIRECTED ) insert( &g->vertices[ finish_idx ], start_idx, 0.0 );
   // si el grafo no es dirigido, entonces insertamos la arista finish-start

   return true;
}


int Graph_GetLen( Graph* g )
{
   return g->len;
}


/**
 * @brief Devuelve la información asociada al vértice indicado.
 *
 * @param g          Un grafo.
 * @param vertex_idx El índice del vértice del cual queremos conocer su información.
 *
 * @return La información asociada al vértice vertex_idx.
 */
Item Graph_GetDataByIndex( const Graph* g, int vertex_idx )
{
   assert( 0 <= vertex_idx && vertex_idx < g->len );

   return g->vertices[ vertex_idx ].data;
}

/**
 * @brief Devuelve una referencia al vértice indicado.
 *
 * Esta función puede ser utilizada con las operaciones @see Vertex_Start(), @see Vertex_End(), @see Vertex_Next().
 *
 * @param g          Un grafo
 * @param vertex_idx El índice del vértice del cual queremos devolver la referencia.
 *
 * @return La referencia al vértice vertex_idx.
 */
Vertex* Graph_GetVertexByIndex( const Graph* g, int vertex_idx )
{
   assert( 0 <= vertex_idx && vertex_idx < g->len );

   return &(g->vertices[ vertex_idx ] );
}

/**
 * @brief Inserta una relación de adyacencia del vértice |start| hacia el vértice |finish| con un peso dado.
 *
 * @param g      El grafo.
 * @param start  Vértice de salida (el dato)
 * @param finish Vertice de llegada (el dato)
 * @param weight Peso de la arista.
 *
 * @return false si uno o ambos vértices no existen; true si la relación se creó con éxito.
 *
 * @pre El grafo no puede estar vacío.
 */
bool Graph_AddWeightedEdge( Graph* g, int start, int finish, float weight )
{
   assert( g->len > 0 );

   // obtenemos los índices correspondientes:
   int start_idx = find( g->vertices, g->size, start );
   int finish_idx = find( g->vertices, g->size, finish );

   DBG_PRINT( "AddWeightedEdge(): from:%d (with index:%d), to:%d (with index:%d), weight:%f\n", start, start_idx, finish, finish_idx, weight );

   if( start_idx == -1 || finish_idx == -1 ) return false;
   // uno o ambos vértices no existen

   insert( &g->vertices[ start_idx ], finish_idx, weight );
   // insertamos la arista start-finish con el peso especificado

   if( g->type == eGraphType_UNDIRECTED ) insert( &g->vertices[ finish_idx ], start_idx, weight );
   // si el grafo no es dirigido, entonces insertamos la arista finish-start con el peso especificado

   return true;
}

/**
 * @brief Indica si un vértice tiene una relación de adyacencia con otro en un grafo no dirigido.
 *
 * @param g     El grafo.
 * @param dest  Vértice de llegada (el dato)
 * @param src   Vértice de salida (el dato)
 *
 * @return true si dest es vecino de src en un grafo no dirigido, false en cualquier otro caso.
 */
bool Graph_IsNeighborOf( Graph* g, int dest, int src )
{
   assert( g->len > 0 );

   // Obtenemos los índices correspondientes:
   int src_idx = find( g->vertices, g->size, src );
   int dest_idx = find( g->vertices, g->size, dest );

   if (src_idx == -1 || dest_idx == -1) {
      return false; // Uno o ambos vértices no existen
   }

   // Verificamos si src tiene una relación de adyacencia con dest
   Vertex* src_vertex = &g->vertices[src_idx];
   if (src_vertex->neighbors) {
      for (List_Cursor_front(src_vertex->neighbors);
           !List_Cursor_end(src_vertex->neighbors);
           List_Cursor_next(src_vertex->neighbors)) {
         Data d = List_Cursor_get(src_vertex->neighbors);
         if (d.index == dest_idx) {
            return true; // dest es vecino de src
         }
      }
   }

   return false; // No se encontró una relación de adyacencia entre src y dest
}


#define MAX_VERTICES 5


int main()
{
    // Crear un grafo para representar la red de aeropuertos
    Graph *grafo = Graph_New(5, eGraphType_DIRECTED); // Utilizamos un digraph

    // Crear aeropuertos con información válida
    Airport airport_MEX = {100, "MEX", "MEXICO", "MEXICO CITY", "AEROPUERTO INTERNACIONAL BENITO JUÁREZ", -6};
    Airport airport_LHR = {120, "LHR", "UNITED KINGDOM", "LONDON", "LONDON HEATHROW", 0}; // Ajusta el UTC Time
    Airport airport_MAD = {130, "MAD", "SPAIN", "MADRID", "MADRID-BARAJAS", 1};          // Ajusta el UTC Time
    Airport airport_FRA = {140, "FRA", "GERMANY", "FRANKFURT", "FLUGHAFEN FRANKFURT AM MAIN", 1}; // Ajusta el UTC Time
    Airport airport_CDG = {150, "CDG", "FRANCE", "PARIS", "CHARLES DE GAULLE", 1};            // Ajusta el UTC Time

    // Agregar los aeropuertos al grafo
    Graph_AddVertex(grafo, airport_MEX);
    Graph_AddVertex(grafo, airport_LHR);
    Graph_AddVertex(grafo, airport_MAD);
    Graph_AddVertex(grafo, airport_FRA);
    Graph_AddVertex(grafo, airport_CDG);

    // Agregar las rutas y tiempos de vuelo (esto es solo un ejemplo, ajusta los valores)
    Graph_AddWeightedEdge(grafo, 100, 120, 9.00);
    Graph_AddWeightedEdge(grafo, 100, 130, 2.50);
    Graph_AddWeightedEdge(grafo, 120, 140, 1.80);
    Graph_AddWeightedEdge(grafo, 130, 150, 1.50);
    Graph_AddWeightedEdge(grafo, 140, 150, 1.20);

    // Imprimir el grafo
    Graph_Print(grafo, 1);

    // Solicitar al usuario un código de vuelo
int flightCode;
while (1)
{
    printf("Ingresa el ID del aeropuerto (100, 120, 130, 140, 150) o -1 para salir: ");
    scanf("%d", &flightCode);

    if (flightCode == -1)
    {
        break;
    }

    // Buscar el vértice correspondiente al ID del aeropuerto
    Vertex *flightVertex = NULL;
    for (int i = 0; i < grafo->len; i++) {
        if (grafo->vertices[i].airport_info.id == flightCode) {
            flightVertex = &grafo->vertices[i];
            break;
        }
    }

    if (flightVertex)
    {
        // Mostrar la información completa del aeropuerto
        printf("Información completa del aeropuerto %d:\n", flightVertex->airport_info.id);
        printf("ID: %d\n", flightVertex->airport_info.id);
        printf("IATA Code: %s\n", flightVertex->airport_info.iata_code);
        printf("Country: %s\n", flightVertex->airport_info.country);
        printf("City: %s\n", flightVertex->airport_info.city);
        printf("Name: %s\n", flightVertex->airport_info.name);
        printf("UTC Time: %d\n", flightVertex->airport_info.utc_time);

        // Mostrar códigos IATA de los vecinos
        printf("Códigos IATA de los vecinos: ");
        Vertex_Start(flightVertex);
        while (!Vertex_End(flightVertex))
        {
            Data neighborData = Vertex_GetNeighborIndex(flightVertex);
            int neighborIndex = neighborData.index;
            printf("%s(W:%.2f) ", grafo->vertices[neighborIndex].airport_info.iata_code, neighborData.weight);
            Vertex_Next(flightVertex);
        }
        printf("\n");
    }
    else
    {
        printf("El aeropuerto con código %d no existe en el grafo.\n", flightCode);
    }
}

    // Liberar la memoria del grafo
    Graph_Delete(&grafo);

    return 0;
}