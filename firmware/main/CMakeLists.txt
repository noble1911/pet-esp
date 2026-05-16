# Main component. Pulls in every feature component (architecture §9).
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES pet_state renderer radio ui audio
)
